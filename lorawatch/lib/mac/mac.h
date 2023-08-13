#ifndef MAC_LAYER_H
#include <LilyGoLib.h>
#include "../DTP/generalsettings.h"
#include <cstdint>
#include <functional>
#include "../RadioLib/src/RadioLib.h"
#include "../mathextension/mathextension.h"
#include <stdexcept>
#define MAC_LAYER_H
#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 3
#define DEFAULT_SPREADING_FACTOR 10
#define DEFAULT_BANDWIDTH 62.5E3
#define DEFAULT_CODING_RATE 2
#define DEFAULT_SQUELCH 15
#define DEFAULT_POWER 10 // dBm
#define NUMBER_OF_MEASUREMENTS 15
#define NUMBER_OF_MEASUREMENTS_LBT 3

#define TIME_BETWEENMEASUREMENTS 50
#define DISCRIMINATE_MEASURMENTS 3

typedef struct {
  uint32_t crc32;
  uint16_t sender;
  uint16_t target;
  unsigned char data[];
} MACPacket;

typedef struct {
  uint32_t crc32;
  uint16_t sender;
  uint16_t target;
} MACHeader;

enum State { IDLE, SENDING, RECEIVING, SLEEPING };

class MAC {
public:
  // Callback function type definition
  using PacketReceivedCallback =
      std::function<void(MACPacket *packet, uint16_t size, uint32_t crcCalculated)>;

  int LORANoiseFloorCalibrate(int channel, bool save = true);
  void LORANoiseCalibrateAllChannels(bool save /*= true*/);
  // Function to access the singleton instance
  static MAC *getInstance();
  static void ChannelActivity(bool signal);
  void setRXCallback(PacketReceivedCallback callback);

  int getNoiseFloorOfChannel(uint8_t channel_num);
  uint8_t getNumberOfChannels();

  // Function to initialize the MAC layer
  static void
  initialize(int id,
    int default_channel = DEFAULT_CHANNEL,
    int default_spreading_factor  = DEFAULT_SPREADING_FACTOR,
    int default_bandwidth = DEFAULT_SPREADING_FACTOR,
    int squelch = DEFAULT_SQUELCH, 
    int default_power = DEFAULT_POWER,
    int default_coding_rate = DEFAULT_CODING_RATE);

  // Function to handle incoming packets or events
  void handlePacket();

  // Function to send packets to the next layer (DTP)
  uint8_t sendData(uint16_t target, unsigned char *data,
                uint8_t size,bool nonblocking, uint32_t timeout = 5000);
  void loop();

  // Other member functions as needed
private:
  static bool transmission_detected;
  static MAC *mac;
  static State state;
  double channels[NUM_OF_CHANNELS] = {433.05, 433.175, 433.3, 433.425,
                                      433.55, 433.675, 433.8, 433.925,
                                      434.05, 434.175, 434.3, 434.425,
                                      434.55, 434.675, 434.8};

  int noiseFloor[NUM_OF_CHANNELS];
  int id;
  int channel;
  int spreading_factor;
  int bandwidth;
  int squelch;
  int power;
  int coding_rate;
  bool readyToReceive;
  // Private constructor
  MAC(int id,
      int default_channel = DEFAULT_CHANNEL,
      int default_spreading_factor = DEFAULT_SPREADING_FACTOR,
      int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH,
      int default_power = DEFAULT_POWER,
      int default_coding_rate = DEFAULT_CODING_RATE);

  // Private destructor
  ~MAC();

  // Private copy constructor and assignment operator to prevent copying
  MAC(const MAC &) = delete;
  MAC &operator=(const MAC &) = delete;
  MACPacket *createPacket(uint16_t sender, uint16_t target, unsigned char *data,
                         uint8_t size);
  static void setMode(State state);
  static State getMode();
  static void RecievedPacket();
  void setFrequencyAndListen(uint16_t);

  bool transmissionAuthorized();
  bool waitForTransmissionAuthorization(uint32_t timeout);

  // Private member variables for MAC layer
  PacketReceivedCallback RXCallback;

  // Private helper functions as needed
};

#endif // MAC_LAYER_H