#include "lcmm.h"
#include "../lora/LoRa-RP2040.h"
#include <vector>
LCMM *LCMM::lcmm = nullptr;
uint16_t LCMM::packetId = 0;
void LCMM::ReceivePacket(MACPacket *packet, uint16_t size, uint32_t crc) {
  if(crc != packet->crc32 || size <= 0){
    return;
  }
  uint8_t type = ((LCMMPacketData *)packet->data)->type;
  switch (type) {
    case PACKET_TYPE_DATA_NOACK:
    case PACKET_TYPE_DATA_ACK:
    case PACKET_TYPE_ACK:
      if(size > sizeof(LCMMPacketResponse)){
        LCMMPacketResponse *response = (LCMMPacketResponse *)packet->data;
        if(response->type == PACKET_TYPE_ACK){
          if(response->numOfPackets == 1){
            if(waitingForACK && ackWaitingSingle.id == response->packetIds[0]){
              ackWaitingSingle.callback(ackWaitingSingle.id, true);
              cancel_repeating_timer(&ackWaitingSingle.timer);
              if(ackWaitingSingle.packet != NULL){
                free(ackWaitingSingle.packet);
              }
              ackWaitingSingle.packet = NULL;
              ackWaitingSingle.callback = NULL;
              waitingForACK = false;
            }
          }          
        }
      }
      break;
    case PACKET_TYPE_PACKET_NEGOTIATION:
    case PACKET_TYPE_PACKET_NEGOTIATION_REFUSED:
    case PACKET_TYPE_PACKET_NEGOTIATION_ACCEPTED:
      break;
    default:
      return;

  }

}

LCMM *LCMM::getInstance() {
  if (lcmm == nullptr) {
    // Throw an exception or handle the error case if initialize() has not been
    // called before getInstance()
  }
  return lcmm;
}

bool LCMM::timeoutHandler(struct repeating_timer *t) {
    if (--ackWaitingSingle.attemptsLeft <= 0 || !waitingForACK) {
        if(waitingForACK){
          ackWaitingSingle.callback(ackWaitingSingle.id, false);
        }
        cancel_repeating_timer(&ackWaitingSingle.timer);
        if(ackWaitingSingle.packet != NULL){
          free(ackWaitingSingle.packet);
        }
        ackWaitingSingle.packet = NULL;
        ackWaitingSingle.callback = NULL;
        waitingForACK = false;
        return false;
    } else {
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
