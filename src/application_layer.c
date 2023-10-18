// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>



unsigned char *buildControlPacket(const int C, long int filesize, const char *filename, int *size){

    //Posição no array
    int pointer = 0;

    //Nº de bytes para o tamanho
    int L1 = ceil((float)filesize/8);

    int L2 = strlen(filename);

    //Tamanho header do control packet
    *size = (1) + (2+L1) + (2+L2);

    //Atribuir tamanho do controlPacket
    unsigned char *controlPacket = malloc(*size * sizeof(unsigned char));

    //Construção do Packet
    controlPacket[pointer++] = C;
    controlPacket[pointer++] = 0;
    controlPacket[pointer] = L1;
    
    //Prencher os bytes de tamanho
    for(char i=0;i<L1;){

        //Prencher a partir do byte menos significativo ao mais significativo
        controlPacket[pointer + L1 - i] = filesize;
        filesize>>=8;
    }

    //Apontar para a parte final do packet e prencher o que falta
    pointer+=L1+1;

    controlPacket[pointer++]=1;
    controlPacket[pointer++]=L2;
    memcpy(controlPacket+pointer,filename,L2);
    return controlPacket;

}

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
        
        //Open file

        FILE *file = fopen(filename,"rb");
        if(file==NULL){
            perror("File doesn't exist.");
            exit(-1);
        }


        //Get file size

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file,0,SEEK_SET);
        printf("%ld\n",fileSize);

        //Send Control Packet START

        int controlSize;
        unsigned char *ControlPacketStart = buildControlPacket(2,fileSize,filename,&controlSize);
        llwrite(ControlPacketStart,controlSize);


        //Send Data Packets

            //...


        //Send Control Packet END
        
        //int controlSize;
        //unsigned char *ControlPacketStart = buildControlPacket(3,fileSize,filename,&controlSize);
        


        break;
    
    case LlRx:

        //Receive Control Packet START

        //Receive Data Packets

        //Close
        break;
    }
}