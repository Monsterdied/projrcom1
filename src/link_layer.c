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

    struct termios oldtio;
// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    printf("terring\n");
}
int nRetransmissions = 1;
int nTimeout = 3;
int fd = 0;
int infoframe = 0;

void update_infoframe()
{
    if (infoframe == 1)
    {
        infoframe = 0;
    }
    else
    {
        infoframe = 1;
    }
}
int close_connection(){
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return 1;
    }
    close(fd);
    return 0;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
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
    State_Read  state = Start_RCV;
    nRetransmissions = connectionParameters.nRetransmissions;
    nTimeout = connectionParameters.timeout;
    if (connectionParameters.role == LlTx)
    {
        unsigned char buf[5];
        memset(&buf, 0, sizeof(5));
        buf[0] = FLAG;
        buf[1] = ADRESS_T;
        buf[2] = SET_CONTROL;
        buf[3] = ADRESS_T ^ SET_CONTROL;
        buf[4] = FLAG;
        int counter = 0;
        while (counter < connectionParameters.nRetransmissions && state != STOP_RCV)
        {
            if (alarmEnabled == FALSE)
            {
                write(fd, buf, 5);
                counter++;
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;
            }
            while (state != STOP_RCV && alarmEnabled == TRUE)
            {
                unsigned char buf_read[1];
                buf_read[0] = 0;
                read(fd, buf_read, 1);
                switch (state) 
                {
                case Start_RCV:

                    if (buf_read[0] == FLAG)
                    {
                        state = Flag_RCV;
                    }
                    break;
                case Flag_RCV:
                    printf("flag\n");

                    if (buf_read[0] == ADRESS_R)
                    {
                        state = A_RCV;
                    }
                    else if (buf_read[0] == FLAG)
                    {
                        state = Flag_RCV;
                    }
                    else
                    {
                        state = Start_RCV;
                    }
                    break;
                case A_RCV:
                    printf("adress\n");
                    if (buf_read[0] == UA_CONTROL)
                    {
                        state = C_RCV;
                    }
                    else if (buf_read[0] == FLAG)
                    {
                        state = Flag_RCV;
                    }
                    else
                    {
                        state = Start_RCV;
                    }
                    break;
                case C_RCV:
                    printf("control\n");
                    if (buf_read[0] == (ADRESS_R ^ UA_CONTROL))
                    {
                        state = BCC_OK;
                    }
                    else if (buf_read[0] == FLAG)
                    {
                        state = Flag_RCV;
                    }
                    else
                    {
                        state = Start_RCV;
                    }
                    break;
                case BCC_OK:
                    printf("bcc\n");
                    if (buf_read[0] == FLAG)
                    {
                        printf("fixe");
                        alarm(0);
                        alarmEnabled = FALSE;
                        state = STOP_RCV;
                    }
                    else
                    {
                        state = Start_RCV;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    else
    {
        printf("read\n");
        while (state != STOP_RCV)
        {
            unsigned char buf_read[1];
            buf_read[0] = 0;
            read(fd, buf_read, 1);
            switch (state)
            {
            case Start_RCV:
                if (buf_read[0] == FLAG)
                {
                    state = Flag_RCV;
                }
                break;
            case Flag_RCV:
                if (buf_read[0] == ADRESS_T)
                {
                    state = A_RCV;
                }
                else if (buf_read[0] == FLAG)
                {
                    state = Flag_RCV;
                }
                else
                {
                    state = Start_RCV;
                }
                break;
            case A_RCV:
                if (buf_read[0] == SET_CONTROL)
                {
                    state = C_RCV;
                }
                else if (buf_read[0] == FLAG)
                {
                    state = Flag_RCV;
                }
                else
                {
                    state = Start_RCV;
                }
                break;
            case C_RCV:
                if (buf_read[0] == (ADRESS_T ^ SET_CONTROL))
                {
                    state = BCC_OK;
                }
                else if (buf_read[0] == FLAG)
                {
                    state = Flag_RCV;
                }
                else
                {
                    state = Start_RCV;
                }
                break;
            case BCC_OK:
                if (buf_read[0] == FLAG)
                {
                    state = STOP_RCV;
                }
                else if (buf_read[0] == FLAG)
                {
                    state = Flag_RCV;
                }
                else
                {
                    state = Start_RCV;
                }
                break;
            default:
                break;
            }
        }
    }
    if (state == STOP_RCV)
    {
        printf("ok\n");
        unsigned char buf[5];
        memset(&buf, 0, sizeof(5));
        buf[0] = FLAG;
        buf[1] = ADRESS_R;
        buf[2] = UA_CONTROL;
        buf[3] = ADRESS_T ^ UA_CONTROL;
        buf[4] = FLAG;
        write(fd, buf, 5);
        return 0;
    }
    printf("rip\n");
    close_connection();
    return -1;
}

unsigned char read_control_frame(unsigned char Adress)
{
    State_Read state = Start_RCV;
    unsigned char control = 0;
    while (alarmEnabled == TRUE && state != STOP_RCV)
    {
        unsigned char buf_read[1];
        buf_read[0] = 0;
        read(fd, buf_read, 1);
        switch (state)
        {
        case Start_RCV:
            if (buf_read[0] == FLAG)
            {
                state = Flag_RCV;
            }
            break;
        case Flag_RCV:
            if (buf_read[0] == Adress)
            {
                state = A_RCV;
            }
            else if (buf_read[0] == FLAG)
            {
                state = Flag_RCV;
            }
            else
            {
                state = Start_RCV;
            }
            break;
        case A_RCV:
            control = buf_read[0];
            if (buf_read[0] == FLAG)
            {
                state = Flag_RCV;
            }
            else
            {
                state = C_RCV;
            }
            break;
        case C_RCV:
            if (buf_read[0] == (Adress ^ control))
            {
                state = BCC_OK;
            }
            else if (buf_read[0] == FLAG)
            {
                state = Flag_RCV;
            }
            else
            {
                state = Start_RCV;
            }
            break;
        case BCC_OK:
            if (buf_read[0] == FLAG)
            {
                alarm(0);
                alarmEnabled = FALSE;
                state = STOP_RCV;
            }
            else
            {
                state = Start_RCV;
            }
            break;
        default:
            break;
        }
    }
    return control;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
        (void)signal(SIGALRM, alarmHandler);
    // We add 6 to the frameSize cause of F(x2) , A , C , BCC1 , BCC2 fields
    int frameSize = bufSize + 6;

    (void)signal(SIGALRM, alarmHandler);

    // We declare the frame and start filling it
    unsigned char *frame = malloc(frameSize * sizeof(unsigned char));

    frame[0] = FLAG;
    frame[1] = ADRESS_R;
    frame[2] = S(infoframe);
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame + 4, buf, bufSize);
    unsigned char BCC2 = buf[0];

    for (int i = 1; i < bufSize; i++)
        BCC2 ^= buf[i];

    // Stuffing
    int counter_escapes = 4;
    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == FLAG)
        {
            counter_escapes++;
            frame = realloc(frame, ++frameSize);
            frame[counter_escapes] = ESC_1;
            frame[counter_escapes++] = ESC_2;
        }
        else if (buf[i] == ESC_1)
        {
            counter_escapes++;
            frame = realloc(frame, ++frameSize);
            frame[counter_escapes] = ESC_1;
            frame[counter_escapes++] = ESC_3;
        }
        else
        {
            frame[counter_escapes++] = buf[i];
        }
    }

    // Finish filling the rest of frame

    frame[counter_escapes++] = BCC2;
    frame[counter_escapes++] = FLAG;

    // 2nd Part of the function

    State_write_t state = Not_Sent;

    int tries = 0;

    while (tries < nRetransmissions && state != Received_Ack)
    {

        // Enable alarm
        if (alarmEnabled == FALSE)
        {
            state = Sent;
            tries++;
            write(fd, frame, frameSize);
            alarm(nTimeout);
            alarmEnabled = TRUE;
        }
        else
        {
            // unsigned char control = read_control_frame(ADRESS_T);

            unsigned char control = read_control_frame(ADRESS_R);
            if (control == REJ_0 || control == REJ_1)
            { // WORK IN PROGRESS
                state = Failed;
                alarm(0);
                alarmEnabled = FALSE;
            }
            else if (control == RR_0 || control == RR_1)
            {
                state = Received_Ack;
                alarm(0);
                alarmEnabled = FALSE;
            }
            else
            {
                state = Failed;
                alarm(0);
                alarmEnabled = FALSE;
            }
        }
    }

    if (state == Received_Ack)
    {
        update_infoframe();
        return frameSize;
    }
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

int sendSupervision(unsigned char A, unsigned char C)
{
    unsigned char frame[5];
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = A ^ C;
    frame[4] = FLAG;
    printf("Going to send SupFrame");
    return write(fd, frame, 5);
}

int llread(unsigned char *packet)
{

    State_read_r state = Start_RCV_r;
    unsigned char readByte;
    unsigned char cByte;
    int size = 0;
    unsigned char bcc2; 
    unsigned char testBCC2;
    while (state != STOP_RCV_r)
    {
        read(fd, &readByte, 1);
        switch (state)
        {
        case Start_RCV_r:
            if (readByte == FLAG)
            {
                state = Flag_RCV_r;
            }
            break;
        case Flag_RCV:
            if (readByte == ADRESS_T)
            {
                state = A_RCV_r;
            }
            else if (readByte != FLAG)
            {
                state = Start_RCV_r;
            }
            break;
        case A_RCV:
            if (readByte == S(0) || readByte == S(1) || readByte == DISC_CONTROL)
            {
                state = C_RCV_r;
                cByte = readByte;
            }
            else if (readByte == FLAG)
            {
                state = Flag_RCV_r;
            }
            else
                state = Start_RCV_r;
            break;
        case C_RCV:
            if (readByte == (ADRESS_T ^ cByte))
            {
                if (cByte == DISC_CONTROL)
                {
                    state = DISC_RCV_r;
                }
                else
                {
                    state = READ_DATA_r;
                }
            }
            else if (readByte == FLAG)
            {
                state = Flag_RCV_r;
            }
            else
                state = Flag_RCV_r;
            break; 
        case DISC_RCV_r:
            if (readByte == FLAG)
            {
                int received = 0 ;
                int tries = 0;
                while ( received == 0 && tries < nRetransmissions)
                {                   
                    sendSupervision(ADRESS_R,DISC_CONTROL);
                    alarm(nTimeout);
                    alarmEnabled = TRUE;
                    unsigned char control = read_control_frame(ADRESS_T);
                    if(control == UA_CONTROL){
                        state = STOP_RCV_r;
                    }
                    else{
                        tries++;
                    }
                }
                
                // close connection not yet implemented
                close_connection();
                return 0;
            }
            else
            {
                state = Start_RCV_r;
            }
            break;
        case READ_DATA_r:
            if (readByte == ESC_1)
            {
                state = ESC_CLEAN_r;
            }
            else if (readByte == FLAG)
            {
                bcc2 = packet[size - 1];
                size--;
                packet[size] = '\0';
                testBCC2 = packet[0];
                for (int i = 1; i < size; i++)
                    testBCC2 ^= packet[i];

                if(testBCC2 == bcc2){
                    sendSupervision(ADRESS_R,infoframe==0?RR_1:RR_0);
                    state = STOP_RCV_r;
                    update_infoframe();
                    return size;
                }
                else{
                    sendSupervision(ADRESS_R,infoframe==0?REJ_0:REJ_1);
                }
            }
            else
            {
                packet[size] = readByte;
                size++;
            }
            break;
        case ESC_CLEAN_r:
            state = READ_DATA_r;
            if (readByte == ESC_2)
            {
                packet[size] = 0x7E;
                size++;
            }
            else if (readByte == ESC_3)
            {
                packet[size] = ESC_1;
                size++;
            }
            else if(readByte == FLAG){
                state = Flag_RCV_r;
                sendSupervision(ADRESS_R,infoframe==0?REJ_0:REJ_1);
            }
            else{
                state = Start_RCV_r;
                sendSupervision(ADRESS_R,infoframe==0?REJ_0:REJ_1);
            }

            break;
        case STOP_RCV_r:
            break;
        }
    }
    close_connection();
    return -1;
}



////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    (void)signal(SIGALRM, alarmHandler);

    int alarmcount = 0;

    while(nRetransmissions != alarmcount){

        sendSupervision(ADRESS_T,DISC_CONTROL);

        alarmEnabled = FALSE;

        alarm(nTimeout);

        unsigned char c = read_control_frame(ADRESS_R);
        if(c == DISC_CONTROL){
            sendSupervision(ADRESS_R,UA_CONTROL);
            close_connection();
            return 0;
        }
        else{
            alarmcount++;
        }
    } 
    close_connection();
    return -1;
}
