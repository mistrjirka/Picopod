#include <stdio.h>
#include "pico/stdlib.h"
#include "../lora/LoRa-RP2040.h"
#include "../mathextension/mathextension.h"
#include "loramessenger.h"
#include <functional>
#include <stdlib.h>
#include <math.h>
#include <vector>

void (*onRecieve)(Packet);

double LoraMessengerClass::channels[15] =
    {433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6, 433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6, 434.8e6};
int LoraMessengerClass::num_of_channels = NUM_OF_CHANNELS;
float LoraMessengerClass::noise_floor_per_channel[15] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
int LoraMessengerClass::current_channel = DEFAULT_CHANNEL;

int LoraMessengerClass::time_between_measurements = TIME_BETWEEN_MEASUREMENTS;
int LoraMessengerClass::squelch = DEFAULT_SQUELCH;
std::vector<Packet> LoraMessengerClass::responseQueue = {};

std::vector<Packet> LoraMessengerClass::sendingQueue = {};
std::vector<PairedDevice> LoraMessengerClass::addressBook = {};
int LoraMessengerClass::current_coding_rate = DEFAULT_CODING_RATE;
int LoraMessengerClass::current_power = DEFAULT_POWER;
int LoraMessengerClass::current_bandwidth = DEFAULT_BANDWIDTH;
int LoraMessengerClass::current_spreading_factor = DEFAULT_SPREADING_FACTOR;
bool LoraMessengerClass::sending = false;
struct repeating_timer LoraMessengerClass::LBTTimer;
struct Packet LoraMessengerClass::current_packet;
int64_t timeoutPacket(alarm_id_t id, void *user_data)
{
    Packet packet = *(Packet *)user_data;
    int index = -1;
    int tmp_index = 0;
    cancel_repeating_timer(&LoraMessengerClass::LBTTimer);
    for (Packet tmp : LoraMessengerClass::responseQueue)
    {
        if (tmp.id == packet.id)
        {
            index = tmp_index;
            break;
        }
        tmp_index++;
    }
    if (index != -1)
    {
        LoraMessengerClass::responseQueue.erase(LoraMessengerClass::responseQueue.begin() + index);
    }
    if (packet.type == COMMUNICATION_PAIRING)
    {
        index = -1;
        tmp_index = 0;
        for (PairedDevice tmp : LoraMessengerClass::addressBook)
        {
            if (tmp.id == packet.id)
            {
                index = tmp_index;
                break;
            }
            tmp_index++;
        }
        if (index != -1)
        {
            LoraMessengerClass::addressBook.erase(LoraMessengerClass::addressBook.begin() + index);
        }
    }

    return 0;
}
bool LBTHandlerCallback(struct repeating_timer *rt)
{
    Packet packet = *(Packet *)rt->user_data;
    LoRa.receive();
    int RSSI = LoRa.rssi();
    if (RSSI <= LoraMessengerClass::noise_floor_per_channel[packet.channel])
    {
        LoRa.endPacket();
        cancel_repeating_timer(rt);
        if (packet.confirmation == true)
        {
            LoraMessengerClass::responseQueue.push_back(packet);
            cancel_alarm(packet.timer_id);
            packet.timer_id = add_alarm_in_ms(packet.timeout, timeoutPacket, &packet, true);
        }
    }

    return 0;
}

void LoraSendPacketLBT(Packet packet) // number one open hailing frequencie!:)
{

    if (packet.confirmation == true)
    {
        packet.timer_id = add_alarm_in_ms(packet.timeout, timeoutPacket, &packet, true);
    }

    // struct repeating_timer timer;

    // LBTHandlerCallback(&timer);
    add_repeating_timer_ms(5000, LBTHandlerCallback, &packet, &LoraMessengerClass::LBTTimer);
}

void LoraMessengerClass::LoraSendPairingRequest(int target_device, int channel) // number one open hailing frequencie!:)
{
    Packet packet;
    packet.target = target_device;
    packet.type = COMMUNICATION_PAIRING;
    packet.confirmation = true;
    packet.incomingType = COMMUNICATION_APPROVED;
    packet.channel = channel;
    packet.id = 0;
    packet.sent = false;
    packet.timeout = 10000;

    int index = -1;
    int tmp_index = 0;
    for (PairedDevice tmp : LoraMessengerClass::addressBook)
    {
        if (tmp.id == target_device)
        {
            index = tmp_index;
            break;
        }
        tmp_index++;
    }
    PairedDevice device;
    if (index == -1)
    {

        device.id = target_device;
        device.my_delay = TIME_ON_AIR_MAX * 1.2;
        device.his_delay = TIME_ON_AIR_MAX * 2.4;
        device.paired = false;

        LoraMessengerClass::addressBook.push_back(device);
    }
    else
    {
        LoraMessengerClass::addressBook.at(index).paired = false;
    }

    LoRa.beginPacket();        // start packet
    LoRa.write(packet.target); // add destination address
    LoRa.write(ID);            // add sender address
    LoRa.write(COMMUNICATION_PAIRING);
    LoRa.write(device.his_delay);
    LoraSendPacketLBT(packet);
}
void LoraAcceptPairingRequest(int target_device, int channel)
{
    Packet packet;
    packet.target = target_device;
    packet.type = COMMUNICATION_PAIRING;
    packet.confirmation = false;
    packet.incomingType = COMMUNICATION_NO_MESSAGE;
    packet.channel = channel;
    packet.id = 0;
    packet.sent = false;
    packet.timeout = 10000;
    LoRa.beginPacket();        // start packet
    LoRa.write(target_device); // add destination address
    LoRa.write(ID);            // add sender address
    LoRa.write(COMMUNICATION_APPROVED);
    LoraSendPacketLBT(packet);
}

int LoraMessengerClass::LORANoiseFloorCalibrate(int channel, bool save /* = true */)
{
    LoRa.idle();

    LoRa.setFrequency(channels[channel]);
    LoRa.receive();
    int noise_measurements[NUMBER_OF_MEASUREMENTS];

    for (int i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
    {
        noise_measurements[i] = LoRa.rssi();
        sleep_ms(time_between_measurements);
    }

    MathExtension.quickSort(noise_measurements, 0, NUMBER_OF_MEASUREMENTS - 1);
    int average = 0;
    for (int i = 0; i < (NUMBER_OF_MEASUREMENTS - DISCRIMINATE_MEASURMENTS); i++)
    {
        average += noise_measurements[i];
    }
    average = average / (NUMBER_OF_MEASUREMENTS - DISCRIMINATE_MEASURMENTS);
    if (save)
    {
        noise_floor_per_channel[channel] = (int)(average + squelch);
    }
    return (int)(average + squelch);
}
void LoraMessengerClass::LORANoiseCalibrateAllChannels(int to_save[NUM_OF_CHANNELS], bool save /*= true*/)
{
    for (int i = 0; i < NUM_OF_CHANNELS; i++)
    {
        to_save[i] = this->LORANoiseFloorCalibrate(i, save);
    }
}
void LoraRecieve(int packetSize)
{
    if (packetSize == -1)
    {
        return; // if there's no packet, return
    }

    // gpio_put(LED_PIN, 1);
    char *message = "";

    // read packet header uint8_ts:
    int recipient = LoRa.read();        // recipient address
    uint8_t sender = LoRa.read();       // sender address
    uint8_t incomingType = LoRa.read(); // incoming msg type

    if (recipient == ID)
    {
        Packet packetToSend;
        int index = -1;
        int tmp_index = 0;
        for (Packet tmp : LoraMessengerClass::responseQueue)
        {
            if (sender == tmp.target && incomingType == tmp.incomingType)
            {
                index = tmp_index;
                break;
            }
            tmp_index++;
        }

        if (index != -1)
        {
            if (incomingType == COMMUNICATION_APPROVED)
            {

                index = -1;
                tmp_index = 0;
                for (PairedDevice tmp : LoraMessengerClass::addressBook)
                {
                    if (tmp.id == sender)
                    {
                        index = tmp_index;
                        break;
                    }
                    tmp_index++;
                }
                if (index == -1)
                {
                    // whoops ID must be corrupted.
                }
                else
                {
                    LoraMessengerClass::addressBook.at(index).paired = true;
                    printf("paired");
                }

                while (LoRa.available())
                {
                    message += (char)LoRa.read();
                }

                packetToSend.content = message;
                packetToSend.type = incomingType;
                packetToSend.channel = LoraMessengerClass::current_channel;
                packetToSend.target = sender;
                (*onRecieve)(packetToSend);
            }
            else if (incomingType == COMMUNICATION_OK_MESSAGE)
            {
                while (LoRa.available())
                {
                    message += (char)LoRa.read();
                }

                packetToSend.content = message;
                packetToSend.type = incomingType;
                packetToSend.channel = LoraMessengerClass::current_channel;
                packetToSend.target = sender;

                (*onRecieve)(packetToSend);
            }
        }
        else
        {

            if (incomingType == COMMUNICATION_PAIRING)
            {
                int delay = (int)LoRa.read();
                index = -1;
                tmp_index = 0;
                for (PairedDevice tmp : LoraMessengerClass::addressBook)
                {
                    if (tmp.id == sender)
                    {
                        index = tmp_index;
                        break;
                    }
                    tmp_index++;
                }
                PairedDevice device;

                if (index == -1)
                {

                    device.id = sender;
                    device.my_delay = delay;
                    device.his_delay = TIME_ON_AIR_MAX * 1.2;
                    device.paired = true;

                    LoraMessengerClass::addressBook.push_back(device);
                }
                else
                {
                    LoraMessengerClass::addressBook.at(index).my_delay = delay;
                }

                LoraAcceptPairingRequest(sender, LoraMessengerClass::current_channel);
                while (LoRa.available())
                {
                    message += (char)LoRa.read();
                }

                packetToSend.content = message;
                packetToSend.type = incomingType;
                packetToSend.channel = LoraMessengerClass::current_channel;
                packetToSend.target = sender;

                (*onRecieve)(packetToSend);
            }
            else if (incomingType == COMMUNICATION_STRING_MESSAGE)
            {
                while (LoRa.available())
                {
                    message += (char)LoRa.read();
                }
                packetToSend.content = message;
                packetToSend.type = incomingType;
                packetToSend.channel = LoraMessengerClass::current_channel;
                packetToSend.target = sender;

                (*onRecieve)(packetToSend);
            }
        }
    }

    LoRa.receive();
}

void LoraMessengerClass::LORASendPacket(Packet packet)
{
    this->sendingQueue.push_back(packet);
};
bool LORASendingDeamon(struct repeating_timer *rt)
{
    if (!LoraMessengerClass::sending && !LoraMessengerClass::sendingQueue.empty())
    {

        LoraMessengerClass::current_packet = LoraMessengerClass::sendingQueue.front();

        if (LoraMessengerClass::current_packet.type == COMMUNICATION_PAIRING)
        {
        }
        else if (LoraMessengerClass::current_packet.type == COMMUNICATION_APPROVED)
        {
        }
        else if (LoraMessengerClass::current_packet.type == COMMUNICATION_STRING_MESSAGE)
        {
        }
    }
}

bool LoraMessengerClass::LORASetup(void (*onRecieveCallback)(Packet), int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/)
{
    if (!LoRa.begin(channels[default_channel]))
    {
        printf("LoRa init failed. Check your connections.\n");
        return false;
    }
    this->current_channel = default_channel;
    this->current_power = default_power;
    this->current_coding_rate = default_coding_rate;
    this->current_bandwidth = default_bandwidth;
    this->current_spreading_factor = default_spreading_factor;
    LoRa.setTxPower(default_power);
    LoRa.setSpreadingFactor(default_spreading_factor);
    LoRa.setSignalBandwidth(default_bandwidth);
    LoRa.setCodingRate4(default_coding_rate);
    this->LORANoiseFloorCalibrate(default_channel);
    LoRa.onReceive(LoraRecieve);
    add_repeating_timer_ms(300, LORASendingDeamon, NULL, &this->ProcessingTimer);

    onRecieve = *onRecieveCallback;
    return true;
}
/*
bool repeating_timer_callback(struct repeating_timer *t)
{
    printf("Repeat at %lld\n", time_us_64());
    return true;
}*/
void test()
{
    LoraSendPacketLBT();
}
LoraMessengerClass LoraMessenger;
