#ifndef MAC_LAYER_H
#include <cstdint>
#define MAC_LAYER_H
#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 3
#define DEFAULT_SPREADING_FACTOR 12
#define DEFAULT_BANDWIDTH 125E3
#define DEFAULT_CODING_RATE 2
#define DEFAULT_SQUELCH 5
#define DEFAULT_POWER 17 // dBm
#define NUMBER_OF_MEASUREMENTS 10
#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS 13
#define PIN_SCK 10
#define PIN_MOSI 11

typedef struct{
    uint16_t sender;
    uint16_t target;
    uint32_t crc32;
    unsigned char data[1024];
} MACPacket;

class MAC {
public:
    static void RecievedPacket(int size);
    // Callback function type definition
    using PacketReceivedCallback = std::function<void(/* Parameters for callback */)>;


    // Function to access the singleton instance
    static MAC* getInstance();

    // Function to initialize the MAC layer
    void initialize(PacketReceivedCallback callback, int id, int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);

    // Function to handle incoming packets or events
    void handlePacket(/* Parameters as per your protocol */);

    // Function to send packets to the next layer (DTP)
    void sendData(/* Parameters as per your protocol */);

    // Other member functions as needed

private:
    static MAC *mac;
    double channels[NUM_OF_CHANNELS];
    int id;
    int channel;
    int spreading_factor;
    int bandwidth;
    int squelch;
    int power;
    int coding_rate;
    // Private constructor
    MAC(PacketReceivedCallback callback, int id, int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);

    // Private destructor
    ~MAC();

    // Private copy constructor and assignment operator to prevent copying
    MAC(const MAC&) = delete;
    MAC& operator=(const MAC&) = delete;

    // Private member variables for MAC layer
    PacketReceivedCallback RXCallback;

    // Private helper functions as needed
};


#endif // MAC_LAYER_H