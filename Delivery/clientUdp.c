#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include "utils.h"
#include <sys/select.h>
#include "send_packet.h"
#include <errno.h>
#include <limits.h>

#define WINDOW_SIZE 7




int main(int argc, char *argv[])
{

if(argc<5){
    printf("Too few arguments, usage: IP Address, Portnumber, Filename, Drop-percentage (integer)\n");
    return 1;
}

char* filenName =argv[3];
char *p ='\0';
int IPaddr = 0;
errno = 0;
long conv = strtol(argv[1], &p, 10);
if (errno != 0 || *p != '\0' || conv > INT_MAX) {
} else {
    
    IPaddr = conv;    
    
}
int portNr=0;
errno = 0;
conv = strtol(argv[2], &p, 10);
if (errno != 0 || *p != '\0' || conv > INT_MAX) {
} else {
    portNr = conv;    
}


float drop = 0;
drop = atof(argv[4])/100 ;

//For stop and wa
fd_set read_set; 
int network_socket, n ;
int length = 0;
network_socket = socket(AF_INET,SOCK_DGRAM, 0 );
//address for the socket
struct sockaddr_in server_adress; 
memset(&server_adress, 0, sizeof(server_adress));
server_adress.sin_family = AF_INET;
server_adress.sin_port = htons(portNr); 
server_adress.sin_addr.s_addr = IPaddr;
length= sizeof(struct sockaddr_in);
//Created socket 
//Select stuffd
struct timeval tv;
char messagebuff[8];
set_loss_probability(drop);
//Serialize an image
int lines[1]; 
struct Packet** packageList =createPackList(argv[3], lines);
int frameCount =0;
int lowerB = 0;
int upperB = 0;
struct Packet* head; 
head = packageList[0];
struct Packet* pointer;
int oldest =0;

 while (1){

    while (upperB - lowerB < WINDOW_SIZE && upperB < lines[0]) 
    {         
        //First send 
        struct Packet* frame = packageList[frameCount];
        char* frameBuf = malloc(sizeof(char) * (frame->totalpacklength +100)); 
        memset(frameBuf, 0 , frame->totalpacklength);       
        serializePackage(frame, frameBuf);
        printf("Sending packet: %d\n", frameBuf[4]);
        
        n= send_packet(network_socket, frameBuf, frame->totalpacklength,0, (struct sockaddr *) &server_adress, length );
        upperB ++;
        frameCount++;
        free(frameBuf);
        //Add to linked list of sent
        if(frame != head)
        {
            pointer = head; 
            while (pointer!=NULL )
            {
                if (pointer->next ==NULL)
                {
                    pointer->next = frame; 
                    break;
                }
                else
                {
                    pointer = pointer->next;
                }            
            }
        }
    }

    //---- Waiting for response
    tv.tv_sec =4;
    tv.tv_usec =0;
    FD_ZERO(&read_set);
    FD_SET(network_socket, &read_set);


    n = select(FD_SETSIZE, &read_set, NULL, NULL, &tv);
    if(n == 0){
        printf("Timed out, no response!\n");
        //Send everything again 
        pointer = head;
        while (pointer!=NULL)
        {
            char* frameBuffer = malloc(sizeof(char) * pointer->totalpacklength);
            memset(frameBuffer, 0 , pointer->totalpacklength);       
            serializePackage(pointer, frameBuffer);          
            printf("Sending packet: %d\n", frameBuffer[4]);
            n= send_packet(network_socket, frameBuffer, pointer->totalpacklength,0, (struct sockaddr *) &server_adress, length );
            pointer = pointer->next;
            free(frameBuffer);
        }        
    }
    //------------------------
    //---- Recieving from server-  
    else  if(FD_ISSET(network_socket, &read_set)){ //event trigge

        n = recvfrom(network_socket, messagebuff, sizeof(messagebuff), 0, (struct sockaddr *) &server_adress, &length);
        printf("ACK sequencenumber recieved: %u\n", messagebuff[5]);     
        if (oldest == messagebuff[5] )
        {
            head = head->next;
            oldest++;
            lowerB++;
        }
        else
        {
            printf("Recieved the wrong ACK, something went very wrong: %d\n", messagebuff[5]);
            return -1;
        } 

    }
    //----------------------------

    //----Termination---
    if(oldest ==(lines[0])){
        //Terminate
        char terminate[8];
        memset(terminate,0,sizeof(terminate));
        ConvertIntToArrayOfUnsignedChar(8 , terminate, 0);
        terminate[6] = 0x4;
        terminate[7] = 0x7f;
        n = send_packet(network_socket, terminate, 8,0, (struct sockaddr *) &server_adress, length );
        printf("All frames sent, terminating connection.\n");
        break;
    }
    //------------------

 }
 
int i= 0;
for (i = 0; i < lines[0]; i++)
{
    freePacket(packageList[i]);
}

free(packageList);
close(network_socket); 


return 0; 
} 
 
