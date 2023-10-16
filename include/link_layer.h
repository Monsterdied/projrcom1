// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum{
Start_RCV,
Flag_RCV,
A_RCV,
C_RCV,
BCC_OK,
READ_DATA,
ESC_CLEAN,
STOP_RCV
}States_Open_t;


typedef enum{
Not_Sent,
Sent,
Failed,
Received_Ack
}State_write_t;


// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

//varaibles of 
#define FLAG 0x7E
#define ADRESS_T 0x03 // Commands by Transmiter or replies from receiver
#define ADRESS_R 0x01 // Commands by Receiver or replies from Transmiter
#define UA_CONTROL 0x07 
#define SET_CONTROL 0x03
#define DISC_CONTROL 0x0B //disconnect signal
#define RR_0 0x05 //receiver ready for frame 0
#define RR_1 0x85 //receiver ready for frame 1
#define REJ_0 0x01 //reject frame 0
#define REJ_1 0x81 //reject frame 1
#define ESC_1 0x7D
#define ESC_2 0x5E
#define ESC_3 0x5D

// MISC
#define FALSE 0
#define TRUE 1

#define S(Ns) (Ns << 6)

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
