// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>

unsigned char *buildControlPacket(ControllPaket*packet){

    //Posição no array
    int pointer = 0;

    //Nº de bytes para o tamanho
    

    int L2 = strlen(packet->filename);
    int L1 =sizeof(packet->filesize);
    //Tamanho header do control packet
    packet->size = (1) + (2+L1) + (2+L2);

    //Atribuir tamanho do controlPacket
    unsigned char *controlPacket = malloc(packet->size);//optimizacao aqui
    //Construção do Packet
    controlPacket[pointer++] = packet->C;
    controlPacket[pointer++] = 0;
    controlPacket[pointer] = L1;
    memcpy(controlPacket+pointer+1,&packet->filesize,L1);
    //Prencher os bytes de tamanho
    /*
    for(char i=0;i<L1;){
        //Prencher a partir do byte menos significativo ao mais significativo
        controlPacket[pointer + L1 - i] = filesize;
        filesize>>=8;
    }
*/
    //Apontar para a parte final do packet e prencher o que falta
    pointer+=L1+1;

    controlPacket[pointer++]=1;
    controlPacket[pointer++]=L2;
    memcpy(controlPacket+pointer,packet->filename,L2);
    return controlPacket;

}


unsigned char *getFileContent(FILE *file, long int filesize){
    unsigned char* fileContent = malloc(filesize);
    fread(fileContent,1,filesize,file);
    return fileContent;
}


ControllPaket readControlPacket(unsigned char *startPacket){
    ControllPaket packet;
    switch(startPacket[0]){
        case 2:
            int Nbytes = startPacket[2];
            memcpy(&packet.filesize,startPacket + 3,Nbytes);
    //for(int i=0; i<Nbytes;i++) *filesize = (startPacket[Nbytes-i-1] <<(8*i));

    //Get File Name
            int Nbytes2 = startPacket[3+Nbytes+1];
    //printf("nbytes2%d\n",Nbytes2);
            packet.filename = (char *)malloc(Nbytes2);
            memcpy(packet.filename,startPacket+3+Nbytes+2,Nbytes2);
            break;
        case 3:
            printf("End Packet\n");
            break;
        default:
            printf("Error\n");
            break;
    }

    return packet;
}


void readDataPacket(unsigned char *dataPacket, unsigned short *data_size,unsigned char *data){
    printf("done\n");
    memcpy(data_size,dataPacket+1,2);
    printf("size %d\n",*data_size);
    memcpy(data,dataPacket+4,*data_size);
    return;
}

void buildDataPacket(unsigned char *dataPacket, unsigned short dataPacket_size, unsigned char *data,int*    packetSize){
    dataPacket[0] = 1;
    memcpy(dataPacket+1,&dataPacket_size,2);
    memcpy(dataPacket+4,data,dataPacket_size);
    *packetSize = dataPacket_size + 4;
    return;
}


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    /*
    int size = 0;
    ControllPaket packet={2,40000000,"ola.txt",0};
    unsigned char* result = buildControlPacket(&packet);
    for(int i=0;i<size;i++){
        printf("%x\n",result[i]);
    }
    ControllPaket packet1 = readControlPacket(result);
    printf("testing\n");
    printf("%s\n",packet1.filename);
    printf("%ld\n",packet1.filesize);*/
    
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
    if(llopen(link)!= -1){
        switch (link.role)
        {
        case LlTx:
    /*
            //test ll
            unsigned char buffer[4] = {'a','b','c','\0'};
            llwrite(buffer, 3);
            int integer = 0;     
            llclose(integer); */

            //Open file
            FILE *file = fopen(filename,"rb");
            if(file==NULL){
                perror("File doesn't exist.");
                exit(-1);
            }
            printf("File opened\n");
            //Get file size
            ControllPaket packetStart;
            packetStart.filename = malloc(strlen(filename));
            strcpy(packetStart.filename,filename);
            packetStart.C = 2;
            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            fseek(file,0,SEEK_SET);
            packetStart.filesize = (long int)filesize;
            printf("%li\n",packetStart.filesize);

            //Send Control Packet START

            unsigned char *ControlPacketStart = buildControlPacket(&packetStart);
            llwrite(ControlPacketStart,packetStart.size);

            //Send Data Packets
            int currentPacket = 0;
            printf("Filesize %li \n",filesize);
            int packetSize = 0;
            while((currentPacket * (MAX_PAYLOAD_SIZE - 4)) < filesize){
                printf("Packet %d\n",currentPacket);
                unsigned char dataPacket[MAX_PAYLOAD_SIZE + 1];
                unsigned char data[MAX_PAYLOAD_SIZE-3];
                unsigned short size = fread(data,1,MAX_PAYLOAD_SIZE-4,file);
                //for(int i=0;i<size;i++){
                //    printf("%c",data[i]);
                //}
                buildDataPacket(dataPacket,size,data,&packetSize);
                /*for(int i=0;i<packetSize;i++){
                    printf("%c",dataPacket[i]);
                }
                printf("print frame import %d\n",size);*/
                if(llwrite(dataPacket,packetSize) == -1){
                    printf("Error\n");
                    break;
                }
                currentPacket ++ ;
            }

            //unsigned char *data = getFileContent(file,fileSize);
            //...
            ControllPaket packetEnd;
            packetEnd.filename= malloc(strlen(filename));
            strcpy(packetEnd.filename,filename);
            packetEnd.C = 3;
            packetEnd.filesize = filesize;
            //Send Control Packet END

            unsigned char *ControlPacketEnd = buildControlPacket(&packetEnd);
            llwrite(ControlPacketEnd,packetEnd.size);
            //Close the connection

            llclose(0);
            free(packetStart.filename);
            free(packetEnd.filename);
            free(ControlPacketStart);
            free(ControlPacketEnd);
            break;
        
        case LlRx:


            //Receive Start Packet

            unsigned char startPacket[MAX_PAYLOAD_SIZE +1];
            int size = llread(startPacket);
            if(size == -1){
                printf("Error\n");
                break;
            }


            //Read Start Packet


            ControllPaket packet = readControlPacket(startPacket);
            printf("received start packet :%s\n",packet.filename);
            //Create a new file to write content
            char path[268];
            strcpy(path, "receive_");
            strcat(path,packet.filename);
            printf("%s\n",path);
            FILE *targetfile = fopen(path,"wb");
            printf("File created\n");
            unsigned char dataPacket[MAX_PAYLOAD_SIZE + 1000];

            //Write into the new file until receive a packet that has packet[0] = 3
            unsigned short size_of_data = 0;
            while(TRUE){
                printf("Reading\n");
                size = llread(dataPacket);
                /*for(int i=0;i<size;i++){
                    printf("%c",dataPacket[i]);
                }*/
                if(dataPacket[0] == 3){
                    llread(dataPacket);
                    printf("CLOSE \n");
                    printf("File closed\n");
                    break;
                } 
                else{
                    //printf("Packet1\n");
                    unsigned char data[997];
                    readDataPacket(dataPacket,&size_of_data,data);
                    //printf("size of data %d\n",size_of_data);
                    //printf("size %ld\n",strlen((char *)data));
                    fwrite(data,1,size_of_data,targetfile);
                    //printf("Packet3\n");
                }
            }
            printf("free stuff\n");
            free(packet.filename);
            fclose(targetfile);
            break;
        }
    }
}