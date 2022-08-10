#pragma once
#include <vector>

#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS 13
#define PIN_SCK 10
#define PIN_MOSI 11
#define CONFIGSET_OFFSET 0
#define CONFIGID_OFFSET 1
#define CONFIGTARGET_OFFSET 2
#define CONFIG_SIZE 3

#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 0
#define DEFAULT_SPREADING_FACTOR 9
#define DEFAULT_BANDWIDTH 125E3
#define DEFAULT_CODING_RATE 2
#define DEFAULT_SQUELCH 5
#define DEFAULT_POWER 17 // dBm
#define NUMBER_OF_MEASUREMENTS 10
#define DISCRIMINATE_MEASURMENTS 0

#define TIME_BETWEEN_MEASUREMENTS 15
#define TIME_ON_AIR_MAX 3000

#define COMMUNICATION_NO_MESSAGE 0
#define ERROR_INVALID_RECIPIENT 1
#define COMMUNICATION_OK_MESSAGE 2
#define COMMUNICATION_STRING_MESSAGE 3
#define COMMUNICATION_PAIRING 4
#define COMMUNICATION_APPROVED 5
#define COMMUNICATION_DECLINED 6
#define COMMUNICATION_CTS 7
#define COMMUNICATION_PAUSE 8
#define COMMUNICATION_FREQUENCY_CHANGE 9
#define COMMUNICATION_SPREADING_FACTOR_CHANGE 10

#define ID 1
struct PairedDevice
{
    int id;
    int my_delay = TIME_ON_AIR_MAX * 1.2;
    int his_delay = TIME_ON_AIR_MAX * 2.4;
    bool paired = false;
};
struct Packet
{
    int type;
    int target;
    int timeout = 1000;
    char *content;
    bool confirmation = true;
    int incomingType;
    int channel;
    int id = -1;
    int timer_id = -1;
    bool sent = false;
};

int64_t timeoutPacket(alarm_id_t id, void *user_data);
bool LBTHandlerCallback(repeating_timer *rt);
extern bool repeating_timer_callback(struct repeating_timer *t);
extern void LoraSendPacketLBT(/*Packet packet*/);
void LoraSendPairingRequest();
void LoraAcceptPairingRequest();
void LoraRecieve(int packetSize);
class LoraMessengerClass
{
private:
    /* data */

    // void LoraAcceptPairingRequest(int target_device, int channel = current_channel);
    // void *LoraRecieve(int packetSize);

    //
    //   void LoraRecieve(int packetSize);
    void LoraAddPacketToRecieve();

public:
    static double channels[15];
    static int num_of_channels;
    static float noise_floor_per_channel[15];
    static int current_channel;
    static int current_power;
    static int current_coding_rate;
    static int current_bandwidth;
    static int current_spreading_factor;
    static Packet current_packet;
    //static std::vector<Packet> responseQueue;
    static std::vector<Packet> sendingQueue;
    static std::vector<PairedDevice> addressBook;
    static int time_between_measurements;
    static int squelch;
    static struct repeating_timer LBTTimer;
    static struct repeating_timer ProcessingTimer;
    static bool sending;
    static int packetId;
    static int LORAGetId();
    int LORANoiseFloorCalibrate(int channel, bool save = true);
    void LORANoiseCalibrateAllChannels(int to_save[NUM_OF_CHANNELS], bool save = true);
    bool LORASetup(void (*onRecieveCallback)(Packet), int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);
};

extern class LoraMessengerClass LoraMessenger;

extern void test();