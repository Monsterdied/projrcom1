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
    if(strcmp(role, "tx") == 0)
        link.role = LlTx;
    else if(strcmp(role, "rx") == 0)
        link.role = LlRx;
    else
        exit(-1);
    link.baudRate = baudRate;

    link.nRetransmissions = nTries;
    link.timeout = timeout;
    printf("linkLayer\n");
    llopen(link);
}
