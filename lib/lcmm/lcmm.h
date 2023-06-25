#ifndef LCMM_LAYER_H
#define LCMM_LAYER_H

// Include necessary headers
#include <functional>
typedef struct{
    u_int8_t type;
    unsigned char[1023] data;
} MACPacketGeneric;

typedef struct{
    uint8_t numOfPackets; // number of packets being acknowledged
    u_int16_t[16] packetIds;
} MACPacketResponse;

typedef struct{
    uint16_t numOfPackets; // number of packets being send
    uint16_t packetid;
    uint8_t ackRate; // number of packets to be sent before waiting for an ack
    u_int16_t packetIdStart; // number of packets to be sent
    unsigned char[256] data;
} MACPacketNegotiation;

typedef struct{
    uint16_t packetid;
    unsigned char[256] data;
} MACPacketNegotiationResponse;

typedef struct{
    uint16_t packetid;
    unsigned char[1021] data;
} MACPacketData;


class LCMM {
public:
    static void RecievedPacket(int size);
    // Callback function type definition
    using PacketReceivedCallback = std::function<void(/* Parameters for callback */)>;

    // Function to access the singleton instance
    static LCMM* getInstance();

    // Function to initialize the LCMM layer
    void initialize(PacketReceivedCallback callback, int id, int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);

    // Function to handle incoming packets or events
    void handlePacket(/* Parameters as per your protocol */);

    // Function to send packets to the next layer (DTP)
    void sendData(/* Parameters as per your protocol */);

    // Other member functions as needed

private:
    static LCMM *lcmm;
    LCMM(PacketReceivedCallback callback, int id, int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);

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
