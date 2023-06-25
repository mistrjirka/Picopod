#include "mac.h"
#include "../lora/LoRa-RP2040.h"

MAC* MAC::mac = nullptr;
double MAC::channels[NUM_OF_CHANNELS] = {
    433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6,
    433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6,
    434.8e6};

static void ReceivedPacket(int size) {
    // Callback function implementation
}

MAC::MAC(PacketReceivedCallback callback, int id,  int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/) {
    this->id = id;
    this->channel = default_channel;
    this->spreading_factor = default_spreading_factor;
    this->bandwidth = default_bandwidth;
    this->squelch = squelch;
    this->power = default_power;
    this->coding_rate = default_coding_rate;
    LoRa.setTxPower(default_power);
    LoRa.setSpreadingFactor(default_spreading_factor);
    LoRa.setSignalBandwidth(default_bandwidth);
    LoRa.setCodingRate4(default_coding_rate);
    // Initialize the LoRa module with the specified settings
    if (!LoRa.begin(channels[default_channel]))
    {
        printf("LoRa init failed. Check your connections.\n");
        return false;
    }

    RXCallback = callback;
}

MAC* MAC::getInstance() {
    if (mac == nullptr) {
        // Throw an exception or handle the error case if initialize() has not been called before getInstance()
    }
    return mac;
}

void MAC::initialize(PacketReceivedCallback callback, int id, int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/) {
    if (mac == nullptr) {
        mac = new MAC(callback, id, default_channel, default_spreading_factor, default_bandwidth, squelch, default_power, default_coding_rate);
    }
}

MAC::~MAC() {
    // Destructor implementation if needed
}

void MAC::handlePacket(/* Parameters as per your protocol */) {
    // Handle incoming packets or events logic
    // You can invoke the stored callback function or pass the provided callback function as needed
    // For example, invoking the stored callback function:
    RXCallback();
}

void MAC::sendData(/* Parameters as per your protocol */) {
    // Send data to the next layer (DTP) logic
    // You can invoke the stored callback function or pass the provided callback function as needed
    // For example, invoking the provided callback function:
    RXCallback();
}

// Other member function implementations as needed
