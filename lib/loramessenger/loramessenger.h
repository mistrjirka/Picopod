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

#define CHANNELS [ 433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6, 433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6, 434.8e6 ]
#define NUM_OF_CHANNELS 15
#define DEFAULT_CHANNEL 0
#define DEFAULT_SPREADING_FACTOR 9
#define DEFAULT_BANDWIDTH 125E3
#define DEFAULT_SQUELCH +5
#define DEFAULT_POWER 17 //dBm

#define ERROR_NO_MESSAGE 0
#define ERROR_INVALID_RECIPIENT 1
#define OK_MESSAGE 2
#define STRING_MESSAGE 3
#define STRING_TELEMETRY_MESSAGE 2

class LoraMessengerClass
{
private:
    /* data */
    static int channels[];
    static int channel;

public:
    bool LORASetup(int default_channel, int default_spreading_factor, int default_bandwidth, int default_power, int squelch);
};
