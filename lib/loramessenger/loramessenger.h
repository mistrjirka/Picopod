#pragma once
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
#define DEFAULT_SQUELCH 5
#define DEFAULT_POWER 17 // dBm
#define NUMBER_OF_MEASUREMENTS 10
#define DISCRIMINATE_MEASURMENTS 0

#define TIME_BETWEEN_MEASUREMENTS 15

#define ERROR_NO_MESSAGE 0
#define ERROR_INVALID_RECIPIENT 1
#define OK_MESSAGE 2
#define STRING_MESSAGE 3
#define COMMUNICATION_HAILING 4
#define COMMUNICATION_APPROVED 5
#define COMMUNICATION_DECLINED 6
#define COMMUNICATION_CTS 7
#define COMMUNICATION_PAUSE 8
#define COMMUNICATION_FREQUENCY_CHANGE 9
#define COMMUNCATION_SPREADING_FACTOR_CHANGE 10

#define ID 1

class LoraMessengerClass
{
private:
    /* data */
    static double channels[15];
    static int num_of_channels;
    static float noise_floor_per_channel[15];
    static int current_channel;

    static int time_between_measurements;
    static int squelch;

public:
    int LORANoiseFloorCalibrate(int channel, bool save = true);
    void LORANoiseCalibrateAllChannels(int to_save[NUM_OF_CHANNELS], bool save = true);
    bool LORASetup(int default_channel = DEFAULT_CHANNEL, int default_spreading_factor = DEFAULT_SPREADING_FACTOR, int default_bandwidth = DEFAULT_BANDWIDTH, int default_power = DEFAULT_POWER, int squelch = DEFAULT_SQUELCH);
};
extern class LoraMessengerClass LoraMessenger;
