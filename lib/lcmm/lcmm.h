#ifndef LCMM_LAYER_H
#define LCMM_LAYER_H
#include <cstdint>
// Include necessary headers
#include "../DTP/generalsettings.h"
#include "../mac/mac.h"
#include <functional>
#include <vector>
 #include "pico/stdlib.h"
using namespace std;
 
#define PACKET_TYPE_DATA_NOACK 0
#define PACKET_TYPE_DATA_ACK 1
#define PACKET_TYPE_DATA_CLUSTER_ACK 2
//#define PACKET_TYPE_DATA_SET 3
#define PACKET_TYPE_ACK 4
#define PACKET_TYPE_PACKET_NEGOTIATION 5
#define PACKET_TYPE_PACKET_NEGOTIATION_REFUSED 6
#define PACKET_TYPE_PACKET_NEGOTIATION_ACCEPTED 7

typedef struct {
  uint8_t type;
  uint16_t packetIds[];
} LCMMPacketResponse;

typedef struct {
  uint8_t type;
  uint16_t id;
  uint8_t ackInterval; // amount of time after which ack is expected from the point the ack to the transmission has been recieved
  uint16_t packetIdStart; // number of packets being send
  uint16_t packetIdEnd; // number of packets to be sent
  unsigned char data[DATASIZE_LCMM - 1 - 2];
} LCMMPacketNegotiation;

typedef struct {
  uint8_t type;
  uint16_t id;
  unsigned char data[DATASIZE_LCMM];
} LCMMPacketNegotiationResponse;

typedef struct {
  uint8_t type;
  uint16_t id;
  unsigned char data[DATASIZE_LCMM];
} LCMMPacketData;


class LCMM {
public:
  static void RecievedPacket(int size);
  // Callback function type definition
  using DataReceivedCallback =
      function<void(int packetId, unsigned char *data, uint32_t size)>;
  using AcknowledgmentCallback =
      function<void(int packetId, bool success)>;
      
 struct ACKWaitingSingle {
    AcknowledgmentCallback callback;
    LCMMPacketData *packet;
    uint32_t timeout;
    uint16_t target;
    uint8_t size;
    uint16_t id;
    uint8_t attemptsLeft;
    struct repeating_timer timer; 
  };
  // Function to access the singleton instance
  static LCMM *getInstance();

  // Function to initialize the LCMM layer
  void initialize(DataReceivedCallback dataRecieved,
                  DataReceivedCallback TransmissionComplete);

  // Function to handle incoming packets or events
  void handlePacket(/* Parameters as per your protocol */);

  void sendPacketLarge(uint16_t target, unsigned char *data, uint32_t size,
                       uint32_t timeout = 50000, uint8_t attempts = 8);

  uint16_t sendPacketSingle(bool needACK, uint16_t target, unsigned char *data,
                        uint8_t size, AcknowledgmentCallback callback,
                        uint32_t timeout = 5000, uint8_t attempts = 3);

  // Other member functions as needed

private:
  static void ReceivePacket(MACPacket *packet, uint16_t size, uint32_t correct);
  static ACKWaitingSingle ackWaitingSingle;
  static bool waitingForACKSingle;
  static uint16_t packetId;
  static LCMM *lcmm;
  static bool timeoutHandler(struct repeating_timer *);
  LCMM(DataReceivedCallback dataRecieved,
       DataReceivedCallback TransmissionComplete);

  // Private destructor
  ~LCMM();

  // Private copy constructor and assignment operator to prevent copying
  LCMM(const LCMM &) = delete;
  LCMM &operator=(const LCMM &) = delete;

  // Private member variables for LCMM layer
  DataReceivedCallback dataReceived;
  DataReceivedCallback transmissionComplete;

  // Private helper functions as needed
};

#endif // LCMM_LAYER_H
