
#ifndef LCMM_LAYER_H
#define LCMM_LAYER_H
/* Link Control and Medium Managment layer */
// Include necessary headers
#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS 13
#define PIN_SCK 10
#define PIN_MOSI 11
#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 3
#define DEFAULT_SPREADING_FACTOR 12
#define DEFAULT_BANDWIDTH 125E3
#define DEFAULT_CODING_RATE 2
#define DEFAULT_SQUELCH 5
#define DEFAULT_POWER 17 // dBm

class LCMM {
public:
    // Callback function type definition
    using PacketRecievedCallback = std::function<void(/* Parameters for callback */)>;

    // Constructor
    LCMM(PacketRecievedCallback callback, int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);

    // Destructor
    ~LCMM();

    // Function to initialize the LCMM layer
    void initialize();

    // Function to handle incoming packets or events
    void handlePacket(/* Parameters as per your protocol */);

    // Function to send packets to the next layer (DTP)
    void sendData(/* Parameters as per your protocol */);

    // Other member functions as needed

private:
    // Private member variables for LCMM layer

    // Private helper functions as needed
};

#endif // LCMM_LAYER_H
