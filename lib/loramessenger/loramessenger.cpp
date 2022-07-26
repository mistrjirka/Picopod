#include <stdio.h>
#include "pico/stdlib.h"
#include "../lora/LoRa-RP2040.h"
#include "../mathextension/mathextension.h"
#include "loramessenger.h"
#include <functional>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string.h>

void (*onRecieve)(RecievedPacket);

double LoraMessengerClass::channels[15] =
    {433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6, 433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6, 434.8e6};
int LoraMessengerClass::num_of_channels = NUM_OF_CHANNELS;
float LoraMessengerClass::noise_floor_per_channel[15] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
int LoraMessengerClass::current_channel = DEFAULT_CHANNEL;

int failed_attempts_rssi = 0;

int LoraMessengerClass::time_between_measurements = TIME_BETWEEN_MEASUREMENTS;
int LoraMessengerClass::squelch = DEFAULT_SQUELCH;
// std::vector<Packet> LoraMessengerClass::responseQueue = {};

std::vector<Packet> LoraMessengerClass::sendingQueue = {};
std::vector<PairedDevice> LoraMessengerClass::addressBook = {};

int LoraMessengerClass::current_coding_rate = DEFAULT_CODING_RATE;
int LoraMessengerClass::current_power = DEFAULT_POWER;
int LoraMessengerClass::current_bandwidth = DEFAULT_BANDWIDTH;
int LoraMessengerClass::current_spreading_factor = DEFAULT_SPREADING_FACTOR;

int LoraMessengerClass::attempt_timer = 0;
int LoraMessengerClass::current_packet_timeout = 0;

int LoraMessengerClass::packetId = 0;

bool LoraMessengerClass::sending = false;

struct repeating_timer LoraMessengerClass::LBTTimer;
struct repeating_timer LoraMessengerClass::ProcessingTimer;
struct Packet LoraMessengerClass::current_packet;

int LoraMessengerClass::searchAdressBook(std::vector<PairedDevice> devices, int id)
{
    int index = -1;
    int tmp_index = 0;
    for (PairedDevice tmp : devices)
    {
        if (tmp.id == id)
        {
            index = tmp_index;
            break;
        }
        tmp_index++;
    }
    return index;
}

int searchPackets(std::vector<Packet> packets, int id)
{
    int index = -1;
    int tmp_index = 0;
    for (Packet tmp : packets)
    {
        if (tmp.id == id)
        {
            index = tmp_index;
            break;
        }
        tmp_index++;
    }
    return index;
}

int LoraMessengerClass::LORAGetId()
{
    LoraMessengerClass::packetId++;
    return LoraMessengerClass::packetId;
}

int64_t timeoutPacket(alarm_id_t id, void *user_data)
{

    int index = -1;
    int tmp_index = 0;
    cancel_repeating_timer(&LoraMessengerClass::LBTTimer);
    LoraMessengerClass::sending = false;
    LoraMessengerClass::current_packet.failed = true;
    if (!LoraMessengerClass::current_packet.retry || !(LoraMessengerClass::current_packet.attempts > MAX_ATTEMPTS))
    {
        for (Packet tmp : LoraMessengerClass::sendingQueue)
        {
            if (tmp.id == LoraMessengerClass::current_packet.id)
            {
                index = tmp_index;
                break;
            }
            tmp_index++;
        }
        if (index != -1)
        {
            LoraMessengerClass::sendingQueue.erase(LoraMessengerClass::sendingQueue.begin() + index);
        }
        if (LoraMessengerClass::current_packet.type == COMMUNICATION_PAIRING)
        {
            index = -1;
            tmp_index = 0;
            for (PairedDevice tmp : LoraMessengerClass::addressBook)
            {
                if (tmp.id == LoraMessengerClass::current_packet.id)
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
    }
    else
    {
        LoraMessengerClass::current_packet.attempts++;
    }
    LoRa.receive();

    return 0;
}

bool LBTHandlerCallback(struct repeating_timer *rt)
{
    LoRa.receive();
    int RSSI = LoRa.rssi();
    if (RSSI <= LoraMessengerClass::noise_floor_per_channel[LoraMessengerClass::current_packet.channel])
    {
        LoRa.endPacket();
        cancel_repeating_timer(rt);
        if (LoraMessengerClass::current_packet.confirmation == true)
        {
            bool state = cancel_alarm(LoraMessengerClass::current_packet_timeout);
            LoraMessengerClass::current_packet_timeout = add_alarm_in_ms(LoraMessengerClass::current_packet.timeout, timeoutPacket, NULL, true);
            LoRa.receive();
        }
        else
        {
            LoraMessengerClass::current_packet.sent = true;
            LoraMessengerClass::sending = false;
            LoRa.receive();
        }
    }
    else
    {
        failed_attempts_rssi++;
        if (failed_attempts_rssi >= 10)
        {
            failed_attempts_rssi = 0;
            LoraMessengerClass::LORANoiseFloorCalibrate(LoraMessengerClass::current_packet.channel, true);
        }
    }

    return true;
}

void LoraSendPacketLBT() // number one open hailing frequencie!:)
{

    if (LoraMessengerClass::current_packet.confirmation == true)
    {
        LoraMessengerClass::current_packet_timeout = add_alarm_in_ms(LoraMessengerClass::current_packet.timeout, timeoutPacket, NULL, true);
    }
    LoRa.idle();
    LoRa.setFrequency(LoraMessengerClass::channels[LoraMessengerClass::current_packet.channel]);
    LoraMessengerClass::current_channel = LoraMessengerClass::current_packet.channel;
    LoRa.receive();

    add_repeating_timer_ms(200, LBTHandlerCallback, NULL, &LoraMessengerClass::LBTTimer);
}

void LoraSendPairingRequest() // number one open hailing frequencie!:)
{
    LoraMessengerClass::current_packet.type = COMMUNICATION_PAIRING;
    LoraMessengerClass::current_packet.confirmation = true;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_APPROVED;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;

    int index = LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, LoraMessengerClass::current_packet.target);

    PairedDevice device;
    if (index == -1)
    {

        device.id = LoraMessengerClass::current_packet.target;
        device.my_delay = TIME_ON_AIR_MAX * 1.2;
        device.his_delay = TIME_ON_AIR_MAX * 2.4;
        device.paired = false;
        LoraMessengerClass::addressBook.push_back(device);
    }
    else
    {
        LoraMessengerClass::addressBook.at(index).paired = false;
    }
    int delay = (int)(device.his_delay / 400);
    LoRa.beginPacket();                                    // start packet
    LoRa.write(LoraMessengerClass::current_packet.target); // add destination address
    LoRa.write(ID);                                        // add sender address
    LoRa.write(COMMUNICATION_PAIRING);
    LoRa.write(delay);
    LoraSendPacketLBT();
}
void LoraAcceptPairingRequest()
{
    LoraMessengerClass::current_packet.confirmation = false;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_NO_MESSAGE;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;
    LoRa.beginPacket();                                    // start packet
    LoRa.write(LoraMessengerClass::current_packet.target); // add destination address
    LoRa.write(ID);                                        // add sender address
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoraSendPacketLBT();
}

void LoraSendStringMessage()
{
    LoraMessengerClass::current_packet.type = COMMUNICATION_STRING_MESSAGE;
    LoraMessengerClass::current_packet.confirmation = true;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_OK_MESSAGE;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;
    LoRa.beginPacket();                                    // start packet
    LoRa.write(LoraMessengerClass::current_packet.target); // add destination address
    LoRa.write(ID);                                        // add sender address
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoRa.write(LoraMessengerClass::current_packet.id);
    LoRa.print(LoraMessengerClass::current_packet.content.c_str());
    LoraSendPacketLBT();
}

void LoraSendOkMessage()
{
    LoraMessengerClass::current_packet.type = COMMUNICATION_OK_MESSAGE;
    LoraMessengerClass::current_packet.confirmation = false;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_NO_MESSAGE;
    LoraMessengerClass::current_packet.sent = false;
    LoRa.beginPacket();                                    // start packet
    LoRa.write(LoraMessengerClass::current_packet.target); // add destination address
    LoRa.write(ID);                                        // add sender address
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoRa.write(LoraMessengerClass::current_packet.id);
    LoraSendPacketLBT();
}

int LoraMessengerClass::LORANoiseFloorCalibrate(int channel, bool save /* = true */)
{
    LoRa.idle();
    LoRa.setFrequency(channels[channel]);
    LoraMessengerClass::current_channel = channel;
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
void LoraMessengerClass::LORANoiseCalibrateAllChannels(bool save /*= true*/)
{
    for (int i = 0; i < NUM_OF_CHANNELS; i++)
    {
        this->LORANoiseFloorCalibrate(i, save);
    }
    LoRa.idle();
    LoRa.setFrequency(channels[LoraMessengerClass::current_channel]);
}
void LoraMessengerClass::LORAAddPacketToQueue(Packet packet)
{
    LoraMessengerClass::sendingQueue.push_back(packet);
};

void LoraMessengerClass::LORAAddPacketToQueuePriority(Packet packet)
{
    LoraMessengerClass::sendingQueue.insert(LoraMessengerClass::sendingQueue.begin(), packet);
};

void LoraMessengerClass::LORACommunicationApproved(RecievedPacket packet)
{
    cancel_alarm(LoraMessengerClass::current_packet_timeout);
    int index = searchAdressBook(LoraMessengerClass::addressBook, packet.sender);
    if (index == -1)
    {
        if (LoraMessengerClass::addressBook.size() == 1)
        {
            LoraMessengerClass::addressBook.at(0).paired = true;
            LoraMessengerClass::sending = false;
        }
    }
    else
    {
        LoraMessengerClass::sending = false;
        LoraMessengerClass::addressBook.at(index).paired = true;
    }

    int indexOfPacket = searchPackets(LoraMessengerClass::sendingQueue, packet.id);
    if (indexOfPacket != -1)
    {
        LoraMessengerClass::sendingQueue.erase(LoraMessengerClass::sendingQueue.begin() + indexOfPacket);
    }

    LoraMessengerClass::current_packet.sent = true;
}

void LoraMessengerClass::LORAPairingRequest(RecievedPacket packet)
{
    int delay = (int)packet.delay * 400;
    int index = searchAdressBook(LoraMessengerClass::addressBook, packet.sender);
    PairedDevice device;

    if (index == -1)
    {

        device.id = packet.sender;
        device.my_delay = delay;
        device.his_delay = TIME_ON_AIR_MAX * 1.2;
        device.paired = true;

        LoraMessengerClass::addressBook.push_back(device);
    }
    else
    {
        LoraMessengerClass::addressBook.at(index).my_delay = delay;
    }

    Packet acceptPacket;
    acceptPacket.target = packet.sender;
    acceptPacket.type = COMMUNICATION_APPROVED;
    acceptPacket.channel = LoraMessengerClass::current_channel;

    LoraMessengerClass::LORAAddPacketToQueuePriority(acceptPacket);
}

void LoraMessengerClass::LORAPacketRecieved(RecievedPacket packet)
{
    if (packet.type == current_packet.incomingType && !LoraMessengerClass::current_packet.failed) // packet expected
    {
        if (packet.type == COMMUNICATION_OK_MESSAGE)
        {
            LoraMessengerClass::sending = false;
            (*onRecieve)(packet);
        }
        else if (packet.type == COMMUNICATION_APPROVED)
        {
            LoraMessengerClass::LORACommunicationApproved(packet);
            (*onRecieve)(packet);
        }
    }
    else
    { // unexpected packet
        if (packet.type == COMMUNICATION_PAIRING)
        {
            LoraMessengerClass::LORAPairingRequest(packet);
            (*onRecieve)(packet);
        }
        else if (packet.type == COMMUNICATION_STRING_MESSAGE)
        {
            Packet OkPacket;
            OkPacket.target = packet.target;
            OkPacket.type = COMMUNICATION_OK_MESSAGE;
            OkPacket.id = packet.id;
            OkPacket.confirmation = false;
            OkPacket.channel = packet.channel;
            LoraMessengerClass::LORAAddPacketToQueuePriority(OkPacket);
            (*onRecieve)(packet);
        }
    }
}

void LoraRecieve(int packetSize)
{
    if (packetSize == -1)
    {
        return; // if there's no packet, return
    }

    RecievedPacket packet;
    packet.channel = LoraMessengerClass::current_channel;
    packet.target = LoRa.read(); // recipient address
    packet.sender = LoRa.read(); // sender address
    packet.type = LoRa.read();   // incoming msg type
    packet.content = "";
    string msg = "";

    if (packet.type == COMMUNICATION_STRING_MESSAGE || packet.type == COMMUNICATION_OK_MESSAGE)
    {
        packet.id = LoRa.read();
    }
    if (packet.type == COMMUNICATION_PAIRING)
    {
        packet.delay = LoRa.read();
    }

    while (LoRa.available())
    {
        char ch = (char)LoRa.read();
        msg.append(1, ch);
        // packet.content += (char)LoRa.read();
    }
    packet.content = const_cast<char *>(msg.c_str());
    packet.RSSI = LoRa.packetRssi();
    packet.SNR = LoRa.packetSnr();

    LoraMessengerClass::LORAPacketRecieved(packet);
    LoRa.receive();
}

int64_t LORASendPacket(alarm_id_t id, void *user_data)
{
    if (LoraMessengerClass::current_packet.type == COMMUNICATION_PAIRING)
    {
        LoraSendPairingRequest();
        LoraMessengerClass::sending = true;
    }
    else if (LoraMessengerClass::current_packet.type == COMMUNICATION_APPROVED)
    {
        LoraAcceptPairingRequest();
        LoraMessengerClass::sending = true;
    }
    else if (LoraMessengerClass::current_packet.type == COMMUNICATION_STRING_MESSAGE)
    {
        LoraSendStringMessage();
        LoraMessengerClass::sending = true;
    }
    else if (LoraMessengerClass::current_packet.type == COMMUNICATION_OK_MESSAGE)
    {
        LoraSendOkMessage();
        LoraMessengerClass::sending = true;
    }
    return 0;
}

bool LoraMessengerClass::LORASendingDeamon(struct repeating_timer *rt)
{
    if (!LoraMessengerClass::sending && !LoraMessengerClass::sendingQueue.empty() && (LoraMessengerClass::current_packet.sent || LoraMessengerClass::current_packet.failed))
    {

        LoraMessengerClass::current_packet = LoraMessengerClass::sendingQueue.front();
        LoraMessengerClass::sendingQueue.erase(LoraMessengerClass::sendingQueue.begin());
        LORASendPacket(LoraMessengerClass::attempt_timer, nullptr);
    }
    else if (!LoraMessengerClass::current_packet.sent && !LoraMessengerClass::sending && LoraMessengerClass::current_packet.retry)
    {
        int index = LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, LoraMessengerClass::current_packet.target);
        if (index == -1)
        {
            return true;
        }
        add_alarm_in_ms(LoraMessengerClass::addressBook.at(index).my_delay, LORASendPacket, NULL, true);
    }
    else if (!LoraMessengerClass::sending)
    {
        LoRa.receive();
    }
    return true;
}

bool LoraMessengerClass::LORASetup(void (*onRecieveCallback)(RecievedPacket), int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/)
{
    LoraMessengerClass::current_packet.sent = true;
    LoraMessengerClass::current_packet.failed = true;
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
    this->LORANoiseCalibrateAllChannels(true);
    LoRa.onReceive(LoraRecieve);
    add_repeating_timer_ms(300, LORASendingDeamon, NULL, &this->ProcessingTimer);

    onRecieve = *onRecieveCallback;
    return true;
}

LoraMessengerClass LoraMessenger;
