// Include standard library for C math functions
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <functional>

// Import Pico libraries
#include "pico/stdlib.h"

// Import LoRa-RP2040 library
#include "../lora/LoRa-RP2040.h"

// Import mathextension library
#include "../mathextension/mathextension.h"

// Import loramessenger library
#include "loramessenger.h"

void (*onRecieve)(RecievedPacket);

// Initialize class variables
double LoraMessengerClass::channels[15] = {
    433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6,
    433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6,
    434.8e6};
int LoraMessengerClass::num_of_channels = NUM_OF_CHANNELS;
float LoraMessengerClass::noise_floor_per_channel[15] = {-100.0};
int LoraMessengerClass::current_channel = DEFAULT_CHANNEL;
int LoraMessengerClass::ID = 0;
int failed_attempts_rssi = 0;
int LoraMessengerClass::time_between_measurements = TIME_BETWEEN_MEASUREMENTS;
int LoraMessengerClass::squelch = DEFAULT_SQUELCH;
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

// Helper function to search paired devices in address book
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

// Helper function to search packets in sending queue
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

// Get unique packet id
int LoraMessengerClass::LORAGetId()
{
    LoraMessengerClass::packetId++;
    return LoraMessengerClass::packetId;
}

// Timer callback function for packet timeout
int64_t timeoutPacket(alarm_id_t id, void *user_data)
{
    cancel_repeating_timer(&LoraMessengerClass::LBTTimer);
    LoraMessengerClass::sending = false;
    LoraMessengerClass::current_packet.failed = true;

    // Remove packet from the sending queue if it hasn't exceeded the maximum number of attempts
    if (!LoraMessengerClass::current_packet.retry || LoraMessengerClass::current_packet.attempts <= MAX_ATTEMPTS)
    {
        auto it = std::find_if(LoraMessengerClass::sendingQueue.begin(), LoraMessengerClass::sendingQueue.end(),
                               [&](const Packet &p)
                               { return p.id == LoraMessengerClass::current_packet.id; });

        if (it != LoraMessengerClass::sendingQueue.end())
        {
            LoraMessengerClass::sendingQueue.erase(it);
        }

        // Remove paired device from address book if the packet is a pairing message
        if (LoraMessengerClass::current_packet.type == COMMUNICATION_PAIRING)
        {
            auto it = std::find_if(LoraMessengerClass::addressBook.begin(), LoraMessengerClass::addressBook.end(),
                                   [&](const PairedDevice &d)
                                   { return d.id == LoraMessengerClass::current_packet.id; });

            if (it != LoraMessengerClass::addressBook.end())
            {
                LoraMessengerClass::addressBook.erase(it);
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

        if (LoraMessengerClass::current_packet.confirmation)
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


/**
 * Sends a LoRa packet with Listen Before Talk (LBT) technique.
 */
void LoraSendPacketLBT()
{
    // If the current packet requires confirmation, set a timeout for it.
    if (LoraMessengerClass::current_packet.confirmation)
    {
        LoraMessengerClass::current_packet_timeout = add_alarm_in_ms(LoraMessengerClass::current_packet.timeout, timeoutPacket, NULL, true);
    }

    // Set the LoRa module to idle mode.
    LoRa.idle();

    // Set the frequency of the LoRa module to the one specified in the current packet's channel.
    LoRa.setFrequency(LoraMessengerClass::channels[LoraMessengerClass::current_packet.channel]);

    // Set the current channel to the one specified in the current packet's channel.
    LoraMessengerClass::current_channel = LoraMessengerClass::current_packet.channel;

    // Set the LoRa module to receive mode.
    LoRa.receive();

    // Add a repeating timer to handle the LBT protocol.
    add_repeating_timer_ms(200, LBTHandlerCallback, NULL, &LoraMessengerClass::LBTTimer);
}

/**
 * Sends a pairing request LoRa packet.
 */
void LoraSendPairingRequest()
{
    // Set the properties of the current packet to match a pairing request.
    LoraMessengerClass::current_packet.type = COMMUNICATION_PAIRING;
    LoraMessengerClass::current_packet.confirmation = true;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_APPROVED;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;

    // Search for the target device in the address book.
    int index = LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, LoraMessengerClass::current_packet.target);

    // If the target device is not in the address book, add it with default delay values.
    PairedDevice device;
    if (index == -1)
    {
        device.id = LoraMessengerClass::current_packet.target;
        device.my_delay = TIME_ON_AIR_MAX * 1.2;
        device.his_delay = TIME_ON_AIR_MAX * 2.4;
        device.paired = false;
        LoraMessengerClass::addressBook.push_back(device);
    }
    // If the target device is already in the address book, mark it as unpaired.
    else
    {
        LoraMessengerClass::addressBook.at(index).paired = false;
    }

    // Calculate the delay for the response packet based on the target device's delay value.
    int delay = (int)(device.his_delay / 400);

    // Start the LoRa packet and add the necessary information to it.
    LoRa.beginPacket();
    LoRa.write(LoraMessengerClass::current_packet.target);
    LoRa.write(LoraMessengerClass::ID);
    LoRa.write(COMMUNICATION_PAIRING);
    LoRa.write(delay);

    // Send the packet using LBT protocol.
    LoraSendPacketLBT();
}
/**
 * Send a packet to accept a pairing request.
 */
void LoraAcceptPairingRequest()
{
    // Set packet fields.
    LoraMessengerClass::current_packet.confirmation = false;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_NO_MESSAGE;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;

    // Begin LoRa packet and add necessary fields.
    LoRa.beginPacket();
    LoRa.write(LoraMessengerClass::current_packet.target);
    LoRa.write(LoraMessengerClass::ID);
    LoRa.write(LoraMessengerClass::current_packet.type);

    // Send packet with LBT technique.
    LoraSendPacketLBT();
}

/**
 * Send a string message packet.
 */
void LoraSendStringMessage()
{
    // Set packet fields.
    LoraMessengerClass::current_packet.type = COMMUNICATION_STRING_MESSAGE;
    LoraMessengerClass::current_packet.confirmation = true;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_OK_MESSAGE;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;

    // Begin LoRa packet and add necessary fields.
    LoRa.beginPacket();
    LoRa.write(LoraMessengerClass::current_packet.target);
    LoRa.write(LoraMessengerClass::ID);
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoRa.write(LoraMessengerClass::current_packet.id);
    LoRa.print(LoraMessengerClass::current_packet.content.c_str());

    // Send packet with LBT technique.
    LoraSendPacketLBT();
}

// Sends an ACK message packet
void LoraSendOkMessage()
{
    // Set packet properties
    LoraMessengerClass::current_packet.type = COMMUNICATION_OK_MESSAGE;
    LoraMessengerClass::current_packet.confirmation = false;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_NO_MESSAGE;
    LoraMessengerClass::current_packet.sent = false;

    // Start building packet
    LoRa.beginPacket();

    // Add destination and sender addresses
    LoRa.write(LoraMessengerClass::current_packet.target);
    LoRa.write(LoraMessengerClass::ID);

    // Add packet type and ID
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoRa.write(LoraMessengerClass::current_packet.id);

    // Send packet using LBT technique
    LoraSendPacketLBT();
}
void LoraSendPayloadMessage()
{
    LoraMessengerClass::current_packet.type = COMMUNICATION_PAYLOAD_MESSAGE;
    LoraMessengerClass::current_packet.confirmation = true;
    LoraMessengerClass::current_packet.incomingType = COMMUNICATION_OK_MESSAGE;
    LoraMessengerClass::current_packet.id = LoraMessengerClass::LORAGetId();
    LoraMessengerClass::current_packet.sent = false;
    LoraMessengerClass::current_packet.timeout = 10000;
    LoRa.beginPacket();                                    // start packet
    LoRa.write(LoraMessengerClass::current_packet.target); // add destination address
    LoRa.write(LoraMessengerClass::ID);                    // add sender address
    LoRa.write(LoraMessengerClass::current_packet.type);
    LoRa.write(LoraMessengerClass::current_packet.id);
    LoRa.print(LoraMessengerClass::current_packet.content.c_str());
    LoraSendPacketLBT();
}

/*
 * LORANoiseFloorCalibrate function calibrates noise floor of the LoRa channel and returns the
 * average value of noise measurements. The noise floor is used to determine the sensitivity
 * threshold of the receiver, i.e., the minimum signal strength required for the receiver to
 * detect a LoRa message. The function takes two arguments: the channel number and a boolean
 * flag indicating whether to save the calibrated noise floor value to the noise_floor_per_channel
 * array. By default, save is true. The function returns an integer value representing the average
 * noise measurement.
 */
int LoraMessengerClass::LORANoiseFloorCalibrate(int channel, bool save /* = true */)
{
    LoRa.idle();                                    // Stop receiving
    LoRa.setFrequency(channels[channel]);           // Set frequency to the given channel
    LoraMessengerClass::current_channel = channel;  // Set current channel
    LoRa.receive();                                 // Start receiving
    int noise_measurements[NUMBER_OF_MEASUREMENTS]; // Array to hold noise measurements

    // Take NUMBER_OF_MEASUREMENTS noise measurements and store in noise_measurements array
    for (int i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
    {
        noise_measurements[i] = LoRa.rssi();
        sleep_ms(time_between_measurements);
    }

    // Sort the array in ascending order using quickSort algorithm
    MathExtension.quickSort(noise_measurements, 0, NUMBER_OF_MEASUREMENTS - 1);

    // Calculate the average noise measurement by excluding the DISCRIMINATE_MEASURMENTS highest values
    int average = 0;
    for (int i = 0; i < (NUMBER_OF_MEASUREMENTS - DISCRIMINATE_MEASURMENTS); i++)
    {
        average += noise_measurements[i];
    }
    average = average / (NUMBER_OF_MEASUREMENTS - DISCRIMINATE_MEASURMENTS);

    // If save is true, save the calibrated noise floor value to noise_floor_per_channel array
    if (save)
    {
        noise_floor_per_channel[channel] = (int)(average + squelch);
    }

    return (int)(average + squelch); // Return the average noise measurement plus squelch value
}

void LoraMessengerClass::LORANoiseCalibrateAllChannels(bool save /*= true*/)
{
    // Calibrate noise floor for all channels and save if requested
    for (int i = 0; i < NUM_OF_CHANNELS; i++)
    {
        this->LORANoiseFloorCalibrate(i, save);
    }
    // Set LoRa to idle and set frequency to current channel
    LoRa.idle();
    LoRa.setFrequency(channels[LoraMessengerClass::current_channel]);
}

void LoraMessengerClass::LORAAddPacketToQueue(Packet packet)
{
    // Add packet to the back of the sending queue
    LoraMessengerClass::sendingQueue.push_back(packet);
};

void LoraMessengerClass::LORAAddPacketToQueuePriority(Packet packet)
{
    // Add packet to the front of the sending queue
    LoraMessengerClass::sendingQueue.insert(LoraMessengerClass::sendingQueue.begin(), packet);
};

void LoraMessengerClass::LORACommunicationApproved(RecievedPacket packet)
{
    // Cancel current packet timeout
    cancel_alarm(LoraMessengerClass::current_packet_timeout);

    // Check if sender is already in address book
    int index = searchAdressBook(LoraMessengerClass::addressBook, packet.sender);
    if (index == -1)
    {
        // If there is only one entry in the address book, mark it as paired
        if (LoraMessengerClass::addressBook.size() == 1)
        {
            LoraMessengerClass::addressBook.at(0).paired = true;
            LoraMessengerClass::sending = false;
        }
    }
    else
    {
        // Mark sender as paired
        LoraMessengerClass::sending = false;
        LoraMessengerClass::addressBook.at(index).paired = true;
    }

    // Remove packet from sending queue
    int indexOfPacket = searchPackets(LoraMessengerClass::sendingQueue, packet.id);
    if (indexOfPacket != -1)
    {
        LoraMessengerClass::sendingQueue.erase(LoraMessengerClass::sendingQueue.begin() + indexOfPacket);
    }

    // Mark current packet as sent
    LoraMessengerClass::current_packet.sent = true;
}

void LoraMessengerClass::LORAPairingRequest(RecievedPacket packet)
{
    // Convert delay to appropriate format
    int delay = (int)packet.delay * 400;

    // Check if the sender is already in the address book
    int index = searchAdressBook(LoraMessengerClass::addressBook, packet.sender);

    // Create a new paired device if the sender is not in the address book
    PairedDevice device;
    if (index == -1)
    {
        device.id = packet.sender;
        device.my_delay = delay;
        device.his_delay = TIME_ON_AIR_MAX * 1.2;
        device.paired = true;

        // Add the new paired device to the address book
        LoraMessengerClass::addressBook.push_back(device);
    }
    else
    {
        // Update the delay of the existing paired device in the address book
        LoraMessengerClass::addressBook.at(index).my_delay = delay;
    }

    // Create an accept packet to send back to the sender
    Packet acceptPacket;
    acceptPacket.target = packet.sender;
    acceptPacket.type = COMMUNICATION_APPROVED;
    acceptPacket.channel = LoraMessengerClass::current_channel;

    // Add the accept packet to the sending queue with priority
    LoraMessengerClass::LORAAddPacketToQueuePriority(acceptPacket);
}
/**

  *  LORAPacketRecieved - Handles the incoming packet, checks the packet type and calls the relevant function accordingly.
*    If the incoming packet is the expected packet, and not a failed packet, it calls the "onRecieve" callback function with the packet data.
 *   If the incoming packet is an unexpected packet, it checks the packet type and calls the relevant function accordingly (pairing request or string message).
  *  @param packet - The received packet to be handled.
*/
void LoraMessengerClass::LORAPacketRecieved(RecievedPacket packet)
{
    if (packet.type == current_packet.incomingType && !LoraMessengerClass::current_packet.failed) // Check if expected packet
    {
        if (packet.type == COMMUNICATION_OK_MESSAGE)
        {
            LoraMessengerClass::sending = false; // Stop sending packets
            (*onRecieve)(packet);                // Notify receiver of the received packet
        }
        else if (packet.type == COMMUNICATION_APPROVED)
        {
            LoraMessengerClass::LORACommunicationApproved(packet); // Mark the packet as sent and remove it from the sending queue
            (*onRecieve)(packet);                                  // Notify receiver of the received packet
        }
    }
    else // Unexpected packet
    {
        if (packet.type == COMMUNICATION_PAIRING)
        {
            LoraMessengerClass::LORAPairingRequest(packet); // Add the sender to the address book and send a communication approved packet back
            (*onRecieve)(packet);                           // Notify receiver of the received packet
        }
        else if (packet.type == COMMUNICATION_STRING_MESSAGE)
        {
            Packet OkPacket;
            OkPacket.target = packet.target;
            OkPacket.type = COMMUNICATION_OK_MESSAGE;
            OkPacket.id = packet.id;
            OkPacket.confirmation = false;
            OkPacket.channel = packet.channel;
            LoraMessengerClass::LORAAddPacketToQueuePriority(OkPacket); // Add an OK packet to the sending queue
            (*onRecieve)(packet);                                       // Notify receiver of the received packet
        }
    }
}

/*

 *   @brief This function reads an incoming LoRa packet and processes it into a ReceivedPacket struct. It then calls LORAPacketRecieved to handle the packet.

 *   @param packetSize The size of the incoming packet, -1 if there is no packet.
    */
void LoraRecieve(int packetSize)
{
    if (packetSize == -1)
    {
        return; // If there's no packet, return
    }

    RecievedPacket packet;
    packet.channel = LoraMessengerClass::current_channel;
    packet.target = LoRa.read(); // Recipient address
    packet.sender = LoRa.read(); // Sender address
    packet.type = LoRa.read();   // Incoming message type
    packet.content = "";
    string msg = "";

    // If the incoming message is a string or an OK message, read the packet ID
    if (packet.type == COMMUNICATION_STRING_MESSAGE || packet.type == COMMUNICATION_OK_MESSAGE)
    {
        packet.id = LoRa.read();
    }
    // If the incoming message is a pairing request, read the delay
    if (packet.type == COMMUNICATION_PAIRING)
    {
        packet.delay = LoRa.read();
    }

    // Read the message content and store it in packet.content
    while (LoRa.available())
    {
        char ch = (char)LoRa.read();
        msg.append(1, ch);
    }
    packet.content = const_cast<char *>(msg.c_str());
    packet.RSSI = LoRa.packetRssi();
    packet.SNR = LoRa.packetSnr();

    // Call LORAPacketRecieved to handle the packet
    LoraMessengerClass::LORAPacketRecieved(packet);
    LoRa.receive();
}

// Sends the current packet over LoRa based on its type
// Sets sending flag to true
int64_t LORASendPacket(alarm_id_t id, void *user_data)
{
    // Determine the type of the current packet and send it accordingly
    switch (LoraMessengerClass::current_packet.type)
    {
    case COMMUNICATION_PAIRING:
        LoraSendPairingRequest();
        break;
    case COMMUNICATION_APPROVED:
        LoraAcceptPairingRequest();
        break;
    case COMMUNICATION_STRING_MESSAGE:
        LoraSendStringMessage();
        break;
    case COMMUNICATION_OK_MESSAGE:
        LoraSendOkMessage();
        break;
    case COMMUNICATION_PAYLOAD_MESSAGE:
        // do nothing
        break;
    }

    LoraMessengerClass::sending = true;
    return 0;
}


/**
 * This function is responsible for managing the sending of LoRa packets.
 * It runs as a repeating timer and checks the sending queue for packets
 * to send. If a packet is ready to be sent, it calls the LORASendPacket
 * function to transmit it. If a packet is not yet ready to be sent, but
 * has been marked for retry, it sets a timer to retry sending the packet
 * later. If no packets are ready to be sent or retried, it puts the
 * LoRa radio in receive mode.
 *
 * @param rt A pointer to the repeating timer struct that triggers this
 *           function.
 * @return true if the function runs successfully, false otherwise.
 */
bool LoraMessengerClass::LORASendingDeamon(struct repeating_timer *rt)
{
    // If we're not currently sending a message, and there are messages waiting in the queue
    // and the current message has been sent or failed, then send the next message in the queue
    if (!LoraMessengerClass::sending && !LoraMessengerClass::sendingQueue.empty() && (LoraMessengerClass::current_packet.sent || LoraMessengerClass::current_packet.failed))
    {
        // Get the next message from the queue, and remove it from the queue
        LoraMessengerClass::current_packet = LoraMessengerClass::sendingQueue.front();
        LoraMessengerClass::sendingQueue.erase(LoraMessengerClass::sendingQueue.begin());

        // Send the message
        LORASendPacket(LoraMessengerClass::attempt_timer, nullptr);
    }
    // If the current message has not been sent, we're not currently sending a message, and the message can be retried
    // then schedule a timer to retry sending the message after the appropriate delay
    else if (!LoraMessengerClass::current_packet.sent && !LoraMessengerClass::sending && LoraMessengerClass::current_packet.retry)
    {
        // Look up the target address in the address book
        int index = LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, LoraMessengerClass::current_packet.target);

        // If the address is not found, return true (not sure what this means, could use a comment)
        if (index == -1)
        {
            return true;
        }

        // Schedule a timer to retry sending the message after the appropriate delay
        add_alarm_in_ms(LoraMessengerClass::addressBook.at(index).my_delay, LORASendPacket, NULL, true);
    }
    // If we're not currently sending a message, wait for incoming messages
    else if (!LoraMessengerClass::sending)
    {
        LoRa.receive();
    }

    // Return true to indicate that the function has executed successfully
    return true;
}

/**
 * Configure and initialize the LoRa radio module.
 *
 * @param onRecieveCallback function pointer to the callback that will handle received packets
 * @param default_channel default channel to use for transmissions
 * @param default_spreading_factor default spreading factor to use for transmissions
 * @param default_bandwidth default signal bandwidth to use for transmissions
 * @param squelch squelch threshold for received packets
 * @param default_power default transmit power to use for transmissions
 * @param default_coding_rate default coding rate to use for transmissions
 *
 * @return true if the LoRa module was successfully configured and initialized, false otherwise
 */
bool LoraMessengerClass::LORASetup(void (*onRecieveCallback)(RecievedPacket), int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/, int default_coding_rate /*DEFAULT_CODING_RATE*/)
{
    // Initialize the current packet status
    LoraMessengerClass::current_packet.sent = true;
    LoraMessengerClass::current_packet.failed = true;

    // Initialize the LoRa module with the specified settings
    if (!LoRa.begin(channels[default_channel]))
    {
        printf("LoRa init failed. Check your connections.\n");
        return false;
    }

    // Set the LoRa module parameters to the specified defaults
    this->current_channel = default_channel;
    this->current_power = default_power;
    this->current_coding_rate = default_coding_rate;
    this->current_bandwidth = default_bandwidth;
    this->current_spreading_factor = default_spreading_factor;
    LoRa.setTxPower(default_power);
    LoRa.setSpreadingFactor(default_spreading_factor);
    LoRa.setSignalBandwidth(default_bandwidth);
    LoRa.setCodingRate4(default_coding_rate);

    // Perform noise calibration on all channels
    this->LORANoiseCalibrateAllChannels(true);

    // Register the receive callback function
    LoRa.onReceive(LoraRecieve);

    // Register the sending daemon function to run every 300ms
    add_repeating_timer_ms(300, LORASendingDeamon, NULL, &this->ProcessingTimer);

    // Register the receive callback function pointer
    onRecieve = *onRecieveCallback;

    return true;
}

LoraMessengerClass LoraMessenger;
