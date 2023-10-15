// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


int alarmEnabled = FALSE;
// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    printf("terring\n");

}
typedef enum{
Start_RCV,
Flag_RCV,
A_RCV,
C_RCV,
BCC_OK,
STOP_RCV
}States_Open_t;

int fd = 0;
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){
    (void)signal(SIGALRM, alarmHandler);
    // Program usage: Uses either COM1 or COM2


    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);



    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }
        printf("begining\n");

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    States_Open_t state = Start_RCV;
    if(connectionParameters.role == LlTx){
        unsigned char buf[5];
        memset(&buf,0,sizeof(5));
        buf[0]=FLAG;
        buf[1]=ADRESS_T;
        buf[2]=SET_CONTROL;
        buf[3]=ADRESS_T^SET_CONTROL;
        buf[4]=FLAG;
        int counter = 0;
        while(counter < connectionParameters.nRetransmissions && state != STOP_RCV){
            if(alarmEnabled == FALSE){
                write(fd, buf, 5);
                counter++;
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;
            }
            while(state != STOP_RCV && alarmEnabled == TRUE){
                unsigned char  buf_read[1];
                buf_read[0] = 0;
                read(fd, buf_read, 1);
                switch (state){
                case Start_RCV:

                    if(buf_read[0]==FLAG){
                        state = Flag_RCV;
                    }
                    break;
                case Flag_RCV:
                                    printf("flag\n");
                    
                    if(buf_read[0]==ADRESS_T){
                        state = A_RCV;
                    }else if(buf_read[0]==FLAG){
                        state = Flag_RCV;
                    }else{
                        state = Start_RCV;
                    }
                    break;   
                case A_RCV:
                printf("adress\n");
                    if(buf_read[0]==UA_CONTROL){
                        state = C_RCV;
                    }else if(buf_read[0]==FLAG){
                        state = Flag_RCV;
                    }else{
                        state = Start_RCV;
                    }
                    break;
                case C_RCV:
                printf("control\n");
                    if(buf_read[0]==(ADRESS_T^UA_CONTROL)){
                        state = BCC_OK;
                    }else if(buf_read[0]==FLAG){
                        state = Flag_RCV;
                    }else{
                        state = Start_RCV;
                    }
                    break;
                case BCC_OK:
                printf("bcc\n");
                    if(buf_read[0]==FLAG){
                        printf("fixe");
                        alarm(0);
                        state = STOP_RCV;
                    }else{
                        state = Start_RCV;
                    }
                    break;
                default:
                    break;
                }

            }
        }
    }else{
        printf("read\n");
        while(state != STOP_RCV){
            unsigned char  buf_read[1];
            buf_read[0] = 0;
            read(fd, buf_read, 1);
            switch (state)
            {
            case Start_RCV:
                if(buf_read[0]==FLAG){
                    state = Flag_RCV;
                }
                break;
            case Flag_RCV:
                if(buf_read[0]==ADRESS_T){
                    state = A_RCV;
                }else if(buf_read[0]==FLAG){
                    state = Flag_RCV;
                }else{
                    state = Start_RCV;
                }
                break;   
            case A_RCV:
                if(buf_read[0]==SET_CONTROL){
                    state = C_RCV;
                }else if(buf_read[0]==FLAG){
                    state = Flag_RCV;
                }else{
                    state = Start_RCV;
                }
                break;
            case C_RCV:
                if(buf_read[0]==(ADRESS_T^SET_CONTROL)){
                    state = BCC_OK;
                }else if(buf_read[0]==FLAG){
                    state = Flag_RCV;
                }else{
                    state = Start_RCV;
                }
                break;
            case BCC_OK:
                if(buf_read[0]==FLAG){
                    state = STOP_RCV;
                }else if(buf_read[0]==FLAG){
                    state = Flag_RCV;
                }else{
                    state = Start_RCV;
                }
                break;
            default:
                break;
            }

        }
    }
        if(state == STOP_RCV){
            printf("ok\n");
            unsigned char buf[5];
            memset(&buf,0,sizeof(5));
            buf[0]=FLAG;
            buf[1]=ADRESS_T;
            buf[2]=UA_CONTROL;
            buf[3]=ADRESS_T^UA_CONTROL;
            buf[4]=FLAG;
            write(fd, buf, 5);
            return 0;
        }
    printf("rip\n");
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
        perror("tcsetattr");
        
        }
        close(fd);
        return -1;

}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO
    

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
