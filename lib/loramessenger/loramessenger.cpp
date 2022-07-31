#include "pico/stdlib.h"
#include "../lora/LoRa-RP2040.h"
#include "../mathextension/mathextension.h"
#include "loramessenger.h"
#include <stdio.h>
#include <math.h>

double LoraMessengerClass::channels[15] =
    {433.05e6, 433.175e6, 433.3e6, 433.425e6, 433.55e6, 433.675e6, 433.8e6, 433.925e6, 434.05e6, 434.175e6, 434.3e6, 434.425e6, 434.55e6, 434.675e6, 434.8e6};
int LoraMessengerClass::num_of_channels = NUM_OF_CHANNELS;
float LoraMessengerClass::noise_floor_per_channel[15] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
int LoraMessengerClass::current_channel = DEFAULT_CHANNEL;

int LoraMessengerClass::time_between_measurements = TIME_BETWEEN_MEASUREMENTS;
int LoraMessengerClass::squelch = DEFAULT_SQUELCH;
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
        to_save[i] = LORANoiseFloorCalibrate(i, save);
    }
}
bool LoraMessengerClass::LORASetup(int default_channel /* = DEFAULT_CHANNEL*/, int default_spreading_factor /* = DEFAULT_SPREADING_FACTOR*/, int default_bandwidth /* = DEFAULT_SPREADING_FACTOR*/, int squelch /*= DEFAULT_SQUELCH*/, int default_power /* = DEFAULT_POWER*/)
{
    if (!LoRa.begin(channels[default_channel]))
    { // initialize ratio at 915 MHz
        printf("LoRa init failed. Check your connections.\n");
        return false;
    }
    current_channel = default_channel;
    LoRa.setTxPower(default_power);
    LoRa.setSpreadingFactor(default_spreading_factor);
    LoRa.setSignalBandwidth(default_bandwidth);
    LORANoiseFloorCalibrate(default_channel);
    return true;
}
LoraMessengerClass LoraMessenger;

/*
const uint LED_PIN = 25;

bool send_telemtery = true;
int ID = 2;
int target_device = 1;

int spreading_factor = 12;
int power_level = 20;
int bandwidth = 41.7E3;

int time_before_confirmation_failed_ms = 2000;
int attempts_before_failing = 10;

int telemetry_update = 6000;

// working values
int lost_packets = 0;
int sent_packets = 0;

char *message;
float voltage;

uint8_t msgCount = 0; // count of outgoing messages

int RSSI = 0;
int SNR = 0;
float noise_floor = -164;
int number_of_measurements = 40;
int time_between_measurements = 40;
int discriminate_measurments = 0;
float squelch = +10;

char *recieved_message = "";
bool recieving = false;

int8_t random_break = 200;

int waiting_ok_id = 0;
bool ok_recieved = false;
bool waiting_overtime = false;
bool ready_to_send_telemetry = true;
int alarm_id = 0;

void LORANoiseFloorCalibrate()
{
    LoRa.receive();
    int noise_measurements[number_of_measurements];

    for (int i = 0; i < number_of_measurements; i++)
    {
        noise_measurements[i] = LoRa.rssi();
        sleep_ms(time_between_measurements);
    }
    int discriminate_noise_measurments[number_of_measurements - discriminate_measurments];
    MathExtension.quickSort(noise_measurements, 0, number_of_measurements - 1);
    float average = 0;
    for (int i = 0; i < (number_of_measurements - discriminate_measurments); i++)
    {
        average += noise_measurements[i];
        discriminate_noise_measurments[i] = noise_measurements[i];
    }
    average = average / (number_of_measurements - discriminate_measurments);
    noise_floor = average + squelch;
    // BluetoothSend("RSSI floor: " + std::to_string(noise_floor) + "dbm");
}

void LORASetup(void)
{
    if (!LoRa.begin(433.1E6))
    { // initialize ratio at 915 MHz
        printf("LoRa init failed. Check your connections.\n");
        while (true)
        {
            gpio_put(LED_PIN, 1);
            sleep_ms(3000);
            gpio_put(LED_PIN, 0);
            sleep_ms(3000);
        }
    }
    LoRa.setTxPower(power_level);
    LoRa.setSpreadingFactor(spreading_factor);
    LoRa.setSignalBandwidth(bandwidth);
    LORANoiseFloorCalibrate();
}
void LORAchangePowerLevel(int powerLevel)
{
    LoRa.setTxPower(powerLevel);
}
void LORAchangeSpreadingFactor(int spreadingFactor)
{
    LoRa.setSpreadingFactor(spreadingFactor);
}

void LORASendOKMessage()
{
    printf("sending OK packet");
    LoRa.beginPacket();           // start packet
    LoRa.write(target_device);    // add destination address
    LoRa.write(ID);               // add sender address
    LoRa.write(msgCount);         // add message ID
    LoRa.write(sizeof("ok") + 1); // add payload length
    LoRa.write(OK_MESSAGE);       // add message type
    LoRa.write(0);                // add message confirmation

    LoRa.print("ok"); //                 // add payload
    LoRa.endPacket();
}
int LORAReceive(int packetSize = -1)
{
    if (LoRa.parsePacket() == 0 && packetSize == -1)
    {
        recieving = false;
        return ERROR_NO_MESSAGE; // if there's no packet, return
    }
    if (packetSize == -1)
    {
        packetSize = LoRa.parsePacket();
    }
    recieving = true;
    // gpio_put(LED_PIN, 1);

    // read packet header uint8_ts:
    int recipient = LoRa.read();          // recipient address
    uint8_t sender = LoRa.read();         // sender address
    uint8_t incomingMsgId = LoRa.read();  // incoming msg ID
    uint8_t incomingLength = LoRa.read(); // incoming msg length
    uint8_t incomingType = LoRa.read();   // incoming msg type
    bool confirmation = LoRa.read();

    char *incoming = "";

    while (LoRa.available())
    {
        incoming += (char)LoRa.read();
    }
    if (sender == waiting_ok_id && incomingType == OK_MESSAGE)
    {
        ok_recieved = true;
        printf("received message");
        ready_to_send_telemetry = true;
        cancel_alarm(alarm_id);
        sent_packets += 1;
    }
    if (recipient != ID && recipient != 0)
    {
        printf("This message is not for me.\n");
        recieving = false;
        gpio_put(LED_PIN, 0);
        ready_to_send_telemetry = true;
        return ERROR_NO_MESSAGE; // if there's no packet,'skip rest of function
    }
    if (confirmation)
    {
        printf("sending ok");
        LORASendOKMessage();
    }
    recieved_message = incoming;
    RSSI = LoRa.packetRssi();
    SNR = LoRa.packetSnr();

    printf("Received from: 0x%x\n", sender);
    printf("Sent to: 0x%x\n", recipient);
    printf("Message ID: %d\n", incomingMsgId);
    printf("Message length: %d\n", incomingLength);
    printf("Message Type: %d\n", incomingType);
    printf("Message confirmation: %d\n", confirmation);
    printf(("Message content: " + incoming + "\n").c_str());
    printf("RSSI: %d\n", RSSI);
    printf("Snr: %d\n", SNR);
    char *toSend = "";
    if (incomingType == OK_MESSAGE)
    {
    }
    else
    {
    }
    //BluetoothSend("\nReceived from: 0x" + std::to_string(sender) + "\n Message type:" + std::to_string(incomingType) + "\n packetSize:" + std::to_string(packetSize) + "\n RSSI:" + std::to_string(RSSI) + "dbm\n SNR: " + std::to_string(SNR) + "\nMessage content" + incoming + "\n");
    // gpio_put(LED_PIN, 0);
    recieving = false;
    ready_to_send_telemetry = true;
    LoRa.receive();
    return incomingType;
}
int64_t readyTelemetryTimer(alarm_id_t id, void *user_data)
{
    ready_to_send_telemetry = true;
    printf("overtime fired\n");
    return 0;
}
int64_t setOvertime(alarm_id_t id, void *user_data)
{
    printf("overtime \n");
    waiting_overtime = true;
    lost_packets += 1; //
    printf("hello there %d\n", lost_packets);
    cancel_alarm(alarm_id);
    random_break = LoRa.random() * 5;
    add_alarm_in_ms(random_break, readyTelemetryTimer, NULL, false);
    // BluetoothSend("Response timout\n");
    return 0;
}
bool LORASendMessage(char *message, int8_t message_type, bool confirmation, bool ignore_trafic)
{
    // printf("sending message type %d, configrmation %d\n", message_type, confirmation);
    int failed_attempts = 0;
    bool sent = false;
    random_break = LoRa.random();
    while (!sent)
    {
        printf("failed attempts %d\n", failed_attempts);
        if (failed_attempts > attempts_before_failing)
        {
            printf("Failed to send a message, too much RF traffic.\n");
            break;
        }
        RSSI = LoRa.rssi();
        printf("\n\nParse Packet: %d recieveing %d ignore %d together %d RSSI %d noise: %f\n\n", noise_floor <= RSSI, recieving, ignore_trafic, ((RSSI <= noise_floor && recieving == false) || ignore_trafic), RSSI, noise_floor);

        if ((RSSI <= noise_floor && recieving == false) || ignore_trafic)
        {

            printf("rf sending\n");
            int n = message.length();
            char messageToSend[n + 1];
            strcpy(messageToSend, message.c_str());

            gpio_put(LED_PIN, 1);
            printf("writing packet");
            LoRa.beginPacket();                    // start packet
            LoRa.write(target_device);             // add destination address
            LoRa.write(ID);                        // add sender address
            LoRa.write(msgCount);                  // add message ID
            LoRa.write(sizeof(messageToSend) + 1); // add payload length

            LoRa.write(message_type);  // add message type
            LoRa.write(confirmation);  // add message confirmation
            LoRa.print(messageToSend); //                 // add payload
            LoRa.endPacket();          // finish packet and send it
            gpio_put(LED_PIN, 0);

            if (confirmation)
            {
                printf("confirmation true\n");
                int RecieveType = 0;

                waiting_overtime = false;
                waiting_ok_id = target_device;

                alarm_id = add_alarm_in_ms(time_before_confirmation_failed_ms, setOvertime, NULL, false);
                ready_to_send_telemetry = false;

                printf("Waiting for response breaking\n");
                break;
            }
            else
            {
                printf("confirmation not true\n");
                sent = true;
                ready_to_send_telemetry = true;
                break;
            }
            failed_attempts++;
            for (int i = 0; random_break * 25 > i; i++)
            {
            }
            random_break = LoRa.random();
        }
        else
        {
            printf("rf traffic");
            failed_attempts++;
            // LORAReceive();
            LoRa.receive();
        }
    }
    // LoRa.receive();
    LoRa.receive();
    printf("ready to send: %d", ready_to_send_telemetry);
    float packetloss;
    if ((lost_packets + sent_packets) != 0)
    {
        packetloss = (float(lost_packets) / float(sent_packets + lost_packets)) * 100;
    }
    else
    {
        packetloss = 2;
    }
    printf(("\n packetloss:  " + std::to_string(packetloss) + " sent packets: " + std::to_string(sent_packets) + "\n lost packets: " + std::to_string(lost_packets) + "\n").c_str());

    return sent;
}
void LORARecieveCallback(int packetsize)
{
    LORAReceive(packetsize);
}*/
