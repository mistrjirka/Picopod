#ifndef LCMM_LAYER_H
#define LCMM_LAYER_H
#include <cstdint>
// Include necessary headers
#include <functional>
#include "../DTP/generalsettings.h"
#include "../mac/mac.h"

#define PACKET_TYPE_DATA_NOACK 0
#define PACKET_TYPE_DATA_ACK 1
#define PACKET_TYPE_DATA_NACK 2
#define PACKET_TYPE_DATA_SET 3

#define PACKET_TYPE_ACK 4
#define PACKET_TYPE_PACKET_NEGOTIATION 5
#define PACKET_TYPE_PACKET_NEGOTIATION_REFUSED 6
#define PACKET_TYPE_PACKET_NEGOTIATION_ACCEPTED 7

typedef struct{
    uint8_t type;
    uint8_t numOfPackets; // number of packets being acknowledged
    uint16_t packetIds[16];
} MACPacketResponse;

typedef struct{
    uint8_t type;
    uint16_t packetid;
    uint16_t numOfPackets; // number of packets being send
    uint8_t ackInterval; // number of packets to be sent before waiting for an ack
    uint16_t packetIdStart; // number of packets to be sent
    unsigned char data[DATASIZE_LCMM - 1 - 2];
} MACPacketNegotiation;

typedef struct{
    uint8_t type;
    uint16_t packetid;
    unsigned char data[DATASIZE_LCMM];
} MACPacketNegotiationResponse;

typedef struct{
    uint8_t type;
    uint16_t packetid;
    unsigned char data[DATASIZE_LCMM];
} MACPacketData;


class LCMM {
public:
    static void RecievedPacket(int size);
    // Callback function type definition
    using DataReceivedCallback = std::function<void(int packetID)>;
    

    // Function to access the singleton instance
    static LCMM* getInstance();

    // Function to initialize the LCMM layer
    void initialize(DataReceivedCallback dataRecieved, DataReceivedCallback TransmissionComplete );

    // Function to handle incoming packets or events
    void handlePacket(/* Parameters as per your protocol */);

    void sendPacketLarge(uint16_t target, unsigned char *data,
                      uint32_t size, uint32_t timeout = 50000);

    void sendPacketSingle(bool needACK, uint16_t target, unsigned char *data,
                      uint8_t size, uint32_t timeout = 5000);

    // Other member functions as needed

private:
    static LCMM *lcmm;
    LCMM(DataReceivedCallback dataRecieved, DataReceivedCallback TransmissionComplete);

    // Private destructor
    ~LCMM();

    // Private copy constructor and assignment operator to prevent copying
    LCMM(const LCMM&) = delete;
    LCMM& operator=(const LCMM&) = delete;

    // Private member variables for LCMM layer
    DataReceivedCallback dataReceived;
    DataReceivedCallback transmissionComplete;

    // Private helper functions as needed
};

#endif // LCMM_LAYER_H
