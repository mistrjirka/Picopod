#include "lcmm.h"
#include "../lora/LoRa-RP2040.h"
#include <vector>
LCMM *LCMM::lcmm = nullptr;
uint16_t LCMM::packetId = 0;
void LCMM::ReceivePacket(MACPacket *packet, uint16_t size, bool damaged) {
}

LCMM *LCMM::getInstance() {
  if (lcmm == nullptr) {
    // Throw an exception or handle the error case if initialize() has not been
    // called before getInstance()
  }
  return lcmm;
}

bool LCMM::timeoutHandler(struct repeating_timer *t) {
    if (--ackWaitingSingle.attemptsLeft <= 0) {
        ackWaitingSingle.callback(ackWaitingSingle.id, false);
        cancel_repeating_timer(&ackWaitingSingle.timer);
        free(ackWaitingSingle.packet);
        ackWaitingSingle.packet = NULL;
        return false;
    }else{
         MAC::getInstance()->sendData(ackWaitingSingle.target, (unsigned char *)ackWaitingSingle.packet,
                               sizeof(LCMMPacketData) + ackWaitingSingle.size, ackWaitingSingle.timeout);
    }
    return true;
}

void LCMM::initialize(DataReceivedCallback dataRecieved,
                      DataReceivedCallback transmissionComplete) {
  if (lcmm == nullptr) {
    lcmm = new LCMM(dataRecieved, transmissionComplete);
  }
  MAC::getInstance()->setRXCallback(LCMM::ReceivePacket);
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
                           uint32_t timeout, uint8_t attempts) {}

uint16_t LCMM::sendPacketSingle(bool needACK, uint16_t target,
                                unsigned char *data, uint8_t size,
                                AcknowledgmentCallback callback,
                                uint32_t timeout, uint8_t attempts) {

  LCMMPacketData *packet = (LCMMPacketData *)malloc(sizeof(LCMMPacketData) + size);
  packet->id = LCMM::packetId++;
  packet->type = needACK ? 1 : 0;
  memcpy((*packet).data, data, size);

  if (needACK) {
    ACKWaitingSingle ackTimer;
    ackTimer.callback = callback;
    ackTimer.timeout = timeout;
    ackTimer.id = packet->id;
    ackTimer.packet = packet;
    ackTimer.attemptsLeft = attempts;
    ackTimer.target = target;
    ackTimer.size = size;
    add_repeating_timer_ms(timeout, timeoutHandler, NULL, &ackTimer.timer);
    ackWaitingSingle = ackTimer;
  }

  MAC::getInstance()->sendData(target, (unsigned char *)packet,
                               sizeof(LCMMPacketData) + size, timeout);
  if(!needACK){
    free(packet);
  }
  return packet->id;
}
