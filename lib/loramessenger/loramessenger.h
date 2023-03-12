#pragma once
#include <vector>
#include <string>
#include <string.h>
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
#define DEFAULT_CHANNEL 3
#define DEFAULT_SPREADING_FACTOR 12
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
#define COMMUNICATION_PAYLOAD_MESSAGE 2

#define COMMUNICATION_STRING_MESSAGE 3
#define COMMUNICATION_PAIRING 4
#define COMMUNICATION_APPROVED 5
#define COMMUNICATION_DECLINED 6
#define COMMUNICATION_CTS 7
#define COMMUNICATION_PAUSE 8
#define COMMUNICATION_FREQUENCY_CHANGE 9
#define COMMUNICATION_SPREADING_FACTOR_CHANGE 10
#define MAX_ATTEMPTS 3


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
    std::string content;
    char* payload;
    bool confirmation = true;
    int incomingType;
    int channel;
    int id = -1;
    bool sent = false;
    bool retry = false;
    bool failed = false;
    int attempts = 0;
};

struct RecievedPacket
{
    int type;
    int target;
    int sender;
    int id;
    int channel;
    int length;
    int delay;
    int RSSI;
    int SNR;
    std::string content;
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
    void LORAAddPacketToRecieve();
    static void LORACommunicationApproved(RecievedPacket packet);
    static void LORAPairingRequest(RecievedPacket packet);

public:
    static int ID;
    static int searchAdressBook(std::vector<PairedDevice> devices, int id);
    static double channels[15];
    static int num_of_channels;
    static float noise_floor_per_channel[15];
    static int current_channel;
    static int current_power;
    static int current_coding_rate;
    static int current_bandwidth;
    static int current_spreading_factor;
    static Packet current_packet;
    // static std::vector<Packet> responseQueue;

    static std::vector<Packet> sendingQueue;
    static std::vector<PairedDevice> addressBook;

    static std::vector<RecievedPacket> recievedPackets;

    static int time_between_measurements;
    static int squelch;
    static struct repeating_timer LBTTimer;
    static struct repeating_timer ProcessingTimer;
    static bool sending;
    static int packetId;

    static int current_packet_timeout;
    static int attempt_timer;

    static int LORAGetId();

    static void LORAAddPacketToQueue(Packet packet);
    static void LORAAddPacketToQueuePriority(Packet packet);

    static int LORANoiseFloorCalibrate(int channel, bool save = true);

    void LORANoiseCalibrateAllChannels(bool save = true);

    static void LORAPacketRecieved(RecievedPacket packet);
    static bool LORASendingDeamon(struct repeating_timer *rt);

    bool LORASetup(void (*onRecieveCallback)(RecievedPacket), int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int squelch = DEFAULT_SQUELCH, int default_power = DEFAULT_POWER, int default_coding_rate = DEFAULT_CODING_RATE);
};

extern class LoraMessengerClass LoraMessenger;

extern void test();