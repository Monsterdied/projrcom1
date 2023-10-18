// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    
    LinkLayer link;

    memset(&link, 0 , sizeof(link));

    strcpy(link.serialPort, serialPort);

    if(strcmp(role, "tx") == 0) link.role = LlTx;
    else if(strcmp(role, "rx") == 0) link.role = LlRx;
    else exit(-1);
    
    link.baudRate = baudRate;
    link.nRetransmissions = nTries;
    link.timeout = timeout;

    printf("linkLayer\n");

    llopen(link);
    switch (link.role)
    {
    case LlTx:  





    
        /*unsigned char buffer[4] = {'a','b','c','\0'};
        llwrite(buffer, 3);
        int integer = 0;     
        llclose(integer); */
        /*FILE *file = fopen(filename,"rb");
        if(file==NULL){
            perror("File doesn't exist.");
            exit(-1);
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file,0,SEEK_SET);
        printf("%ld\n",fileSize);
*/
        break;
    
    case LlRx:




            /*unsigned char ok[100];
            int j = 0;
            while((j = llread(ok)) >0){
                printf("j = %d\n",j);
                for(int i = 0; i < j;i++){
                    printf("%c",ok[i]);
                }
                printf("\n");
            }*/
            
        break;
    }
}
