#ifndef DTP_H
#define DTP_H
#include <stdint.h>
typedef struct  {
    uint16_t neighborID;
    uint8_t range;
} NeighborRecord;
typedef struct{
    uint16_t originalSender;
    uint16_t finalTarget;
    uint16_t id;
    uint8_t type; // 0 = NAP, 1 = DATA, 2 = ACK, 3 = NACK - target not in database, 4 = NACK - target unresponsive
    unsigned char data[1014];
} DTPPacketGeneric;

typedef struct{
    uint8_t length;
    NeighborRecord *neighbors;
} DTPPacketNAP;

typedef struct{
    uint8_t length;
    uint16_t *proxies;
} DTPPacketNACK;

#endif // DTP_H