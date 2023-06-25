#include "lcmm.h"
#include "../lora/LoRa-RP2040.h"



LCMM* LCMM::lcmm = nullptr;

static void ReceivedPacket(int size) {
    // Callback function implementation
}

LCMM::LCMM(PacketReceivedCallback callback, int id,  int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/) {

}

LCMM* LCMM::getInstance() {
    if (lcmm == nullptr) {
        // Throw an exception or handle the error case if initialize() has not been called before getInstance()
    }
    return lcmm;
}

void LCMM::initialize(PacketReceivedCallback callback, int id, int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/) {
    if (lcmm == nullptr) {
        lcmm = new LCMM(callback, id, default_channel, default_spreading_factor, default_bandwidth, squelch, default_power, default_coding_rate);
    }
}

LCMM::~LCMM() {
    // Destructor implementation if needed
}

void LCMM::handlePacket(/* Parameters as per your protocol */) {
    // Handle incoming packets or events logic
    // You can invoke the stored callback function or pass the provided callback function as needed
    // For example, invoking the stored callback function:
    RXCallback();
}

void LCMM::sendData(/* Parameters as per your protocol */) {
    // Send data to the next layer (DTP) logic
    // You can invoke the stored callback function or pass the provided callback function as needed
    // For example, invoking the provided callback function:
    RXCallback();
}

// Other member function implementations as needed
