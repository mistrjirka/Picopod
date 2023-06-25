#ifndef LCMM_LAYER_H
#define LCMM_LAYER_H
#include <cstdint>
// Include necessary headers
#include <functional>
#include "../DTP/gerealsettings.h"

typedef struct{
    uint8_t type;
    unsigned char data[DATASIZE_LCMM + 2];
} MACPacketGeneric;

typedef struct{
    uint8_t numOfPackets; // number of packets being acknowledged
    uint16_t packetIds[16];
} MACPacketResponse;

typedef struct{
    uint16_t numOfPackets; // number of packets being send
    uint16_t packetid;
    uint8_t ackRate; // number of packets to be sent before waiting for an ack
    uint16_t packetIdStart; // number of packets to be sent
    unsigned char data[DATASIZE_LCMM - 1 - 2];
} MACPacketNegotiation;

typedef struct{
    uint16_t packetid;
    unsigned char data[DATASIZE_LCMM];
} MACPacketNegotiationResponse;

typedef struct{
    uint16_t packetid;
    unsigned char data[DATASIZE_LCMM];
} MACPacketData;


class LCMM {
public:
    static void RecievedPacket(int size);
    // Callback function type definition
    using PacketReceivedCallback = std::function<void(/* Parameters for callback */)>;

    // Function to access the singleton instance
    static LCMM* getInstance();

    // Function to initialize the LCMM layer
    void initialize(PacketReceivedCallback callback);

    // Function to handle incoming packets or events
    void handlePacket(/* Parameters as per your protocol */);

    // Function to send packets to the next layer (DTP)
    void sendData(/* Parameters as per your protocol */);

    // Other member functions as needed

private:
    static LCMM *lcmm;
    LCMM(PacketReceivedCallback callback);

    // Private destructor
    ~LCMM();

    // Private copy constructor and assignment operator to prevent copying
    LCMM(const LCMM&) = delete;
    LCMM& operator=(const LCMM&) = delete;

    // Private member variables for LCMM layer
    PacketReceivedCallback RXCallback;

    // Private helper functions as needed
};

#endif // LCMM_LAYER_H
