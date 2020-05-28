#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#define constHeadSize 16




struct Packet{     
    int totalpacklength;
    unsigned char pack_seqnum, last_seqnum, flag;
    char* payload;
    int filNameLen;
    char* filename;
    int imgLength;
    int debug;
    struct Packet* next;
};



struct imageFile
{
    struct Image* image; 
    char* file;
    
};

//This function simply prints the bits of the given variabl
void printBits(size_t const size, void const * const ptr);


//A function that takes an integer an adds it to a character array over 4 indexes (4 bytes) 
//Little endian
void ConvertIntToArrayOfUnsignedChar(int iIntToConvert, unsigned char* arruc, int start);


//Convert litte endian 4 bytes of char into a single int 
void ConvertLE4CharToInt(int *convertInt, unsigned char* buffer, int offset);

//Write to file based on whether it matches c
void writeToFile(char* payloadName, char* localName, char* filename, int matchInt);


//Gets Only imagebuffe from file
char * getImgBuffFromFile(char* filename); 

//Frees a packet and the malloced payload
void freePacket(struct Packet* pack);

//Gets a packet struct from proivded filename
struct Packet* getPackFromFile(char* filename,   unsigned char pack_seqnum, unsigned char last_seqnum); 

struct Packet** createPackList(char *fileNameFile, int* lnPtr);


void serializePackage( struct Packet* pack, char* buffer);

struct Packet* deSerialize(char* buffer);

struct imageFile* getImagesFromDir(char* dirName, int *size);

void freeImageFile(struct imageFile* imgFile);

void freeImageList(struct imageFile* imageList, int range);



#endif


