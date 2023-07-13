#include "lcmm.h"
#include "../lora/LoRa-RP2040.h"

LCMM *LCMM::lcmm = nullptr;

static void ReceivedPacket(MACPacket *packet, uint16_t size) {
  // Callback function implementation
}

LCMM *LCMM::getInstance() {
  if (lcmm == nullptr) {
    // Throw an exception or handle the error case if initialize() has not been
    // called before getInstance()
  }
  return lcmm;
}

void LCMM::initialize(DataReceivedCallback dataRecieved,
                      DataReceivedCallback transmissionComplete) {
  if (lcmm == nullptr) {
    lcmm = new LCMM(dataRecieved, transmissionComplete);
  }
  MAC::getInstance()->setRXCallback(ReceivedPacket);
}
LCMM::LCMM(DataReceivedCallback dataRecieved,
           DataReceivedCallback transmissionComplete) {
  this->dataReceived = dataRecieved;
  this->transmissionComplete = transmissionComplete;
}

LCMM::~LCMM() {
  // Destructor implementation if needed
}

void LCMM::handlePacket(/* Parameters as per your protocol */) {
  // Handle incoming packets or events logic
  // You can invoke the stored callback function or pass the provided callback
  // function as needed For example, invoking the stored callback function:
}

void LCMM::sendPacketLarge(uint16_t target, unsigned char *data, uint32_t size,
                           uint32_t timeout = 50000) 
{

}

void LCMM::sendPacketSingle(bool needACK, uint16_t target, unsigned char *data,
                            uint8_t size, uint32_t timeout = 5000) 
{
    
}

/*
void LCMM::sendData(/* Parameters as per your protocol ) {
    // Send data to the next layer (DTP) logic
    // You can invoke the stored callback function or pass the provided callback
function as needed
    // For example, invoking the provided callback function:
}

// Other member function implementations as needed
*/