#ifndef MAC_LAYER_H
#include "../DTP/generalsettings.h"
#include <cstdint>
#include <functional>
#define MAC_LAYER_H
#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 3
#define DEFAULT_SPREADING_FACTOR 10
#define DEFAULT_BANDWIDTH 62.5E3
#define DEFAULT_CODING_RATE 2
#define DEFAULT_SQUELCH 5
#define DEFAULT_POWER 10 // dBm
#define NUMBER_OF_MEASUREMENTS 10
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

enum State { IDLE, SENDING, RECEIVING, SLEEPING, SIGNAL_DETECTION };

class MAC {
public:
  static void RecievedPacket(int size);
  // Callback function type definition
  using PacketReceivedCallback =
      std::function<void(MACPacket *packet, uint16_t size, uint32_t crcCalculated)>;

  int LORANoiseFloorCalibrate(int channel, bool save = true);
  void LORANoiseCalibrateAllChannels(bool save /*= true*/);
  // Function to access the singleton instance
  static MAC *getInstance();
  static void ChannelActivity(bool signal);
  void setRXCallback(PacketReceivedCallback callback);

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
  void handlePacket(uint16_t size);

  // Function to send packets to the next layer (DTP)
  uint8_t sendData(uint16_t target, unsigned char *data,
                uint8_t size,bool nonblocking, uint32_t timeout = 5000);

  // Other member functions as needed
private:
  static bool transmission_detected;
  static MAC *mac;
  static State state;
  double channels[NUM_OF_CHANNELS] = {433.05e6, 433.175e6, 433.3e6, 433.425e6,
                                      433.55e6, 433.675e6, 433.8e6, 433.925e6,
                                      434.05e6, 434.175e6, 434.3e6, 434.425e6,
                                      434.55e6, 434.675e6, 434.8e6};

  int noiseFloor[NUM_OF_CHANNELS];
  int id;
  int channel;
  int spreading_factor;
  int bandwidth;
  int squelch;
  int power;
  int coding_rate;
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
  bool transmissionAuthorized();
  bool waitForTransmissionAuthorization(uint32_t timeout);

  // Private member variables for MAC layer
  PacketReceivedCallback RXCallback;

  // Private helper functions as needed
};

#endif // MAC_LAYER_H