#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include "pgmread.h"



void writeToFile(char* payloadName, char* localName, char* filename, int matchInt){

    FILE * fp;
    int i;
    fp = fopen (filename,"ab+");
    char writing[100]; 
    fseek(fp, 0, SEEK_END);

    if (matchInt == 1)
    {
        sprintf(writing, "%s %s\n", localName, payloadName );
        printf( "%s" , writing);
        fprintf( fp, "%s" ,writing);

    }else
    {
        sprintf(writing, "%s UNKNOWN\n",payloadName);
        printf( "%s" , writing);
        fprintf( fp, "%s" ,writing);
    }
    
    fclose (fp);
}



void ConvertLE4CharToInt(int *convertInt, unsigned char* buffer, int offset){
        *convertInt = buffer[0+offset]  | (buffer[1+offset] << 8) | (buffer[2+offset] << 16) | (buffer[3+offset] << 24);

}



void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=0;i<=size;i++)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}


void ConvertIntToArrayOfUnsignedChar(int iIntToConvert, unsigned char* arruc, int start)
{
    int i;
    unsigned u, u1;
    for (i = 0; i < sizeof(int); i++) {
        u = iIntToConvert;
        u = u >> (8 * i);
        u1 = u & 0xff;
        arruc[start] = (unsigned char)u1;
        start++;
    }
}


char* getImgBuffFromFile(char* filename) 
{
    
    
    FILE * fnp;
    fnp = fopen(filename , "r");
    if (fnp == NULL) {
        perror("Failed: fnp Open failed ");
    }

    struct stat st;

    //Check if the file is valid
    stat(filename, &st);

    int sz = st.st_size;
    int flnLen = strlen(basename(filename))+1;

    char imgBuffer[sz + flnLen + 1 + 8];
    char* testBuffer = malloc(sz + flnLen + 1 + 8);
    printf("Size: %d\n", sz);
    fread(imgBuffer, 1, sz, fnp);
    printf("Filename length: %d\n",flnLen);

    sprintf(testBuffer, "%u%u%u%s%s", sz , 0, flnLen, basename(filename), imgBuffer );
    fclose(fnp);

    printf("Size of imgBuffer: %ld\n", sizeof(imgBuffer));


    return testBuffer;
}



void freePacket(struct Packet* pack)
{
    if(pack==NULL) return;
    if(pack->payload) free(pack->payload);
    if(pack->filename) free(pack->filename);
    free(pack);   
}


struct Packet* getPackFromFile(char* filename, unsigned char pack_seqnum, unsigned char last_seqnum) 
{
    
    FILE * fnp;
    fnp = fopen(filename , "r");
    if (fnp == NULL) {
        perror("Failed: fnp Open failed ");
        return NULL;
    }

    struct stat st;

    //Initialize the pack
    struct Packet *pack =malloc(sizeof(struct Packet));
    memset(pack, 0, sizeof(struct Packet));

    //Check if the file is valid
    stat(filename, &st);

    int sz = st.st_size;
    int flnLen = strlen(basename(filename));

    char* imgBuffer = malloc( sizeof(char) * sz);
    fread(imgBuffer, 1, sz, fnp);

    unsigned char flag = 0x1;
    pack -> totalpacklength = (sz + constHeadSize +flnLen);
    pack -> imgLength = sz; 
    pack -> pack_seqnum = pack_seqnum; 
    pack -> last_seqnum = last_seqnum;
    pack -> flag = flag;
    pack ->  payload = imgBuffer;
    pack -> filename = strdup(basename(filename));
    pack -> filNameLen = flnLen;

    fclose(fnp);
    return pack;

}



struct Packet** createPackList(char *fileNameFile, int* lnPtr)
{
    
    FILE * fp;
    char * line = NULL;
    int lines = 0;
    size_t len = 0;
    size_t read;  
    struct stat st;
    fp= fopen(fileNameFile, "r");
    if (fp == NULL)
        perror("Failed: fnp Open failed ");

    //check if file is valid
    stat(fileNameFile,&st );

    


    char filenames[100][100] = {{0}};
    while ((read = getline(&line, &len, fp)) != -1) 
    {
    
        strncpy(filenames[lines] , line, strlen(line)-1);
        lines++;
    }
    fclose(fp);    
    lnPtr[0] =lines;
    free(line);

    struct Packet** packetList = malloc(sizeof(struct Packet) * lines);
    int i =0;
    for ( i = 0; i < lines; i++)
    {   
        struct Packet* pack = getPackFromFile(filenames[i], i, 0);
        pack->pack_seqnum = i; 
        packetList[i] = pack;
        pack->next = NULL;
    }
    

    return packetList;
}; 



//Function that "deserializes" i.e converts the bytes in the buffer recieved from client
//into a packet struct

struct Packet* deSerialize(char* buffer)
{
    //Deserialize the header
    int count, filenameLen, debug;
    filenameLen =0;
     
    ConvertLE4CharToInt(&count, buffer, 0);
    struct Packet *pack = malloc(sizeof(struct Packet));
    memset(pack, 0, sizeof(struct Packet));

    pack->totalpacklength = count;
    pack->pack_seqnum = buffer[4];
    pack->last_seqnum = buffer[5];
    pack->flag = buffer[6];

    pack->debug = debug;

    //Desiralize (int) filenameLength
    ConvertLE4CharToInt(&filenameLen, buffer, 12);
    pack->filNameLen = filenameLen;
    
    //Deserialize (char*) filename
    char* filenameBuf = malloc(sizeof(char) * filenameLen +1);
    memset(filenameBuf, 0, filenameLen);
    int i =0;
    for (i = 0; i < filenameLen; i++)
    {
        filenameBuf[i] = buffer[i + 16]; 
    }
    filenameBuf[filenameLen]= '\0';
    pack->filename = filenameBuf;

   
    //Derialize Image and imagelength
    int bufcount = filenameLen + 16;
    int imgLen = count -bufcount;
    pack->imgLength = imgLen;
    char* imgBuf= malloc(sizeof(char) *imgLen +1);
    memset(imgBuf, 0, imgLen);
    for (i = 0; i < imgLen; i++)
    {   
        imgBuf[i] = buffer[i+bufcount];
    }
    pack->payload = imgBuf;    

    return pack;
}



void serializePackage( struct Packet* pack, char* buffer)
{
    ConvertIntToArrayOfUnsignedChar( pack->totalpacklength , buffer, 0 );
    buffer[4] = pack->pack_seqnum;
    buffer[5] = pack->last_seqnum;
    buffer[6] = pack->flag;
    buffer[7] = 0x7f;
    //Adding the payload
    //Adding (int) unique number of request
    ConvertIntToArrayOfUnsignedChar( pack->debug , buffer, 8);
    
    //Adding (int) length of the filename
    ConvertIntToArrayOfUnsignedChar( pack->filNameLen , buffer, 12);
    int bufCount = 16;
    int count =0;
    
    //Adding the filename 
    int i =0;
    for (i = 0; i < pack->filNameLen; i++)
    {
        
        buffer[i+bufCount] = pack->filename[count];
        count ++;
    }
    bufCount += count;   
    //Adding Image
    for (i = 0; i < pack->imgLength; i++)
    {
        buffer[i+bufCount] = pack->payload[i];
    }  

}


void freeImageFile(struct imageFile* imgFile){

    if(imgFile==NULL) return;
    if(imgFile->image)Image_free(imgFile->image);
    if(imgFile->file) free(imgFile->file);
}


void freeImageList(struct imageFile* imageList, int range){

    int i;
    for (i = 0; i < range; i++)
    {    
        freeImageFile(&imageList[i]);
    }

    free(imageList);

}

//Remember to free
struct imageFile* getImagesFromDir(char* dirName, int *size){


    //Go back and find a count to amount of images
    struct imageFile *imageList = malloc(sizeof(struct imageFile) *1 );
    char filename[300];
    DIR *dp; 
    dp = opendir(dirName);
    struct dirent *de;

    if (dp == NULL)
    {
        printf("Error opendir\n");
    }
    
    //NB! readdir does not sorter alphabetically 
    int count =0;
    while ((de = readdir(dp))  !=NULL )
    {
        struct stat stbuf;

        if( !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") ){
            //do nothing
            printf("Skipping directory dot\n");
            
        }
        //Extend cond to test it's image file?
        else{

            sprintf( filename , "%s/%s",dirName,de->d_name) ;
        
            if( stat(filename,&stbuf ) == -1 )
                {
                    printf("Unable to stat file: %s\n",filename);
                    continue ;
                }

                FILE * fnp;
                //printf("name: %s \n", filename);
                fnp = fopen(filename , "r");
                if (fnp == NULL) {
                    perror("Failed: fnp Open failed ");
                }  
                //Find better way to allocate size of buff
                struct Image *imgPointer;
                char imgBuffer[stbuf.st_size];
                fread(imgBuffer, 1, stbuf.st_size, fnp);
                imgPointer = Image_create(imgBuffer);
                char *file;
                file = strdup(basename(filename));

                struct imageFile imgFile = {imgPointer, file};
                imageList= realloc(imageList ,sizeof(struct imageFile) * (1+count));
                imageList[count] = imgFile;
                //printf("%d: %s \n", count, filename);
                count++;
                fclose(fnp);
            }
                
    }

    *size = count;

    closedir(dp);
    return imageList;

}

