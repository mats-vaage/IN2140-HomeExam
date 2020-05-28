#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include "utils.h"
#include "pgmread.h"
#include <errno.h>
#include <limits.h>


void handle_payload(char* buffer, struct imageFile* imageList, int range, char* writeFileName){

   struct Packet* frame= deSerialize(buffer);
   printf("Recieved frame of: %d size and %d seck_num\n and name: %s\n", frame->totalpacklength, frame->pack_seqnum, frame->filename);
   struct Image* img = Image_create(frame->payload);

   int anyMatch =0;
   int i;
   for (i = 0; i < range; i++)
   {
      if(Image_compare(imageList[i].image, img) == 1 ){
         writeToFile(frame->filename, imageList[i].file, writeFileName, 1);
         printf("Writing match\n");
         anyMatch = 1;
         
      } 
   } 
   if(anyMatch == 0){
      writeToFile(frame->filename, NULL, writeFileName, 0);
   }
   Image_free(img);
   freePacket(frame);
   //printf("Sucessfully deserialized! %d\n", img->height)
}



int main(int argc, char  *argv[])
{
 

if(argc<4){
    printf("Too few arguments, usage: Portnumber, Directory, Filename\n");
    return 1;
}

char* dirName = argv[1];
char* writeFileName = argv[3];
char *p;
int portNr;
errno = 0;
long conv = strtol(argv[1], &p, 10);

if (errno != 0 || *p != '\0' || conv > INT_MAX) {

} else {
    
    portNr = conv;    
    printf("%d\n", portNr);
}

int files; 
struct imageFile* imageList = getImagesFromDir(argv[2], &files);
//Creating socket
int server_socket;
server_socket = socket(AF_INET,SOCK_DGRAM, 0 );
if (server_socket  < 0)
{
   printf("Error opening socket");
}

//address for the socket
struct sockaddr_in server_adress, clientaddr; 
socklen_t clientaddrlen;
server_adress.sin_family = AF_INET;
//Adding portnumber
server_adress.sin_port = htons(portNr); 
server_adress.sin_addr.s_addr = INADDR_ANY;
//bind the socket to specified IP and portt
if ( bind(server_socket, (struct sockaddr *)&server_adress,sizeof(server_adress) )  < 0)
 {
    printf("Error binding");
 }

int len, n; 
len = sizeof(clientaddr);  //len is value/result 

//Decided to hard-code the buffer size to 3000, although the max size can be much greater 
char* buffer = malloc(3000); 
char ack[8];
int ackLength = sizeof(ack);
bzero(ack, sizeof(ack));
ack[7] = 127;
ConvertIntToArrayOfUnsignedChar(ackLength, ack, 0); 


int expected =0;

while(1){

   n = recvfrom(server_socket, buffer, 3000,  0, ( struct sockaddr *) &clientaddr, &len); 


   //-----Recieving frame
   if(!(buffer[6] & 0x4 )){
      
      if (buffer[4]!= expected)
      {
         //Do nothing
         printf("Recieved seqnum: %d, expected: %d\n", buffer[4], expected);
      }else
      {
         //Passing to application layer
         handle_payload(buffer, imageList, files,  writeFileName);
         printf("Sending back ack_num: %d\n", buffer[4]);
         //Sending ACK
         ack[5] = buffer[4];
         ack[6] = 0x2;
         sendto(server_socket, (const char* )ack, sizeof(ack), 0, (struct sockaddr *) &clientaddr, len); 
         expected ++;
      }
   }
   //--------------------

   
   //----Terminating
   if(buffer[6] & 0x4){
      printf("Closing connection\n");
      int j;
      for ( j = 0; j < files; j++)
      {
         freeImageFile(&imageList[j]);
      }
      
      free(imageList);
      close(server_socket);
      free(buffer); 
      return 0;
      
   }
   //----------------
   
}



return 0;
} 




