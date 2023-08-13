#include <stdio.h>
#include "pico/stdlib.h"
#include <cstdlib>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "Picopod.h"
#include "lib/mathextension/mathextension.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "lib/bluetooth/bluetooth.h"
#include "lib/voltage/voltage.h"
#include "lib/mac/mac.h"
#include "lib/lora/LoRa-RP2040.h"
#include "lib/lcmm/lcmm.h"
#include "lib/IOUSBBT/IOUSBBT.h"
#include <hardware/flash.h>
#include <iostream>
#include <algorithm>

/*void lol(RecievedPacket packet)
{
    printf("{\"type\": %d, \"target\": %d, \"id\": %d, \"content\": \"%s\", \"rssi\":%d, \"snr\":%d, \"sender\": %d}\n", packet.type, packet.target, packet.id, packet.content.c_str(), packet.RSSI, packet.SNR, packet.sender);
    // std::cout << "{\"type\":" << packet.type << ",\"target\":" << packet.target << ",\"id\"" << packet.id << ",\"content\"" << packet.content << ",\"rssi\":" << packet.RSSI << ",\"SNR\":" << packet.SNR << "}";
}*/

void getCommand(std::string *command, int *param)
{
    printf("%s", (*command).c_str());
    std::cin.sync();

    std::cin >> (*command) >> (*param);
    std::cin.ignore();
}

void getMessage(std::string *message)
{
    std::getline(std::cin, *message, '>');
}

/*Packet load_packet_information(bool message)
{
    Packet packet;
    packet.type = getchar() - '0';
    packet.incomingType = getchar() - '0';
    packet.target = getchar() - '0';
    packet.id = getchar() - '0';
    packet.channel = getchar() - '0';
    if (message)
        getMessage(&(packet.content));
    packet.content.erase(remove(packet.content.begin(), packet.content.end(), '\r'), packet.content.end());
    return packet;
}*/

int getNumber()
{
    char buffer[MAX_NUMBER_LENGTH];
    char tmp = '\n';
    for (int i = 0; i < MAX_NUMBER_LENGTH - 1 && (tmp = getchar()) != '\r' && tmp != '\n'; i++)
        buffer[i] = tmp;
    buffer[MAX_NUMBER_LENGTH - 1] = '\0';
    return atoi(buffer);
}

void setup()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    // Voltage.initVoltage(26, 2);

    // LoraMessenger.LORASetup(lol);
}

/*void sendPacket(Packet packet)
{
    if (packet.channel >= 0 && packet.channel <= 20)
    {
        //LoraMessengerClass::LORAAddPacketToQueue(packet);
        printf("{\"type\":-1}");
    }
    else
    {
        printf("{\"type\":-666}");
    }
}*/

void listenForCommands()
{
    char startCommands = getchar_timeout_us(0);
    if (startCommands == 's')
    {
        // Packet packet = load_packet_information(true);
        // sendPacket(packet);
        tight_loop_contents();
    }
    else if (startCommands == 'g')
    {
        // printf("{\"type\":-3, \"data\": \"%d\"}", LoraMessengerClass::ID);
    }
    else if (startCommands == 'c')
    {
        int id = getchar_timeout_us(100000);
        if (id == PICO_ERROR_TIMEOUT)
        {
            printf("{\"type\":-2}");
        }
        else
        {
            // LoraMessengerClass::ID = id - '0';
        }
    }
    else if (startCommands == 'h')
    {
        /*Packet packet = load_packet_information(false);
        int length = getNumber();
        if (length < 0)
        {
            printf("{\"type\":-667}");
        }
        else
        {
            packet.payload = (char *)malloc(length * sizeof(char));
            for (int i = 0; i < length; i++)
            {
                packet.payload[i] = getchar();
            }
        }
        sendPacket(packet);*/
    }
    else if (startCommands == 'p')
    {
        // Packet packet = load_packet_information(false);
        // sendPacket(packet);
    }
}

int main()

{
    stdio_init_all();
        sleep_ms(3000);

    // setup();
    // Define the callback function

    MAC::PacketReceivedCallback dataCallback = [](MACPacket *packet, uint16_t size, uint32_t crcCalculated)
    {
        // Perform actions with the received packet and size
        // For example, print the packet data to the console
        printf("Received packet from %d to %d with packet type: \n", packet->sender, packet->target);
        for (int i = 0; i < size; i++)
        {
            printf("%c \n", packet->data[i]);
        }
        printf("\n");
        if(packet){
            free(packet);
            packet = NULL;
        }
    };
    /*LCMM::DataReceivedCallback dataCallback = [](LCMMPacketDataRecieve *packet, uint32_t size)
    {
        // Perform actions with the received packet and size
        // For example, print the packet data to the console
        printf("Received packet from %d to %d with packet type: %d: \n", packet->mac.sender, packet->mac.target, packet->type);
        for (int i = 0; i < size; i++)
        {
            printf("%c \n", packet->data[i]);
        }
        printf("\n");
        if(packet){
            free(packet);
            packet = NULL;
        }
    };*/
    LCMM::AcknowledgmentCallback ackCallback = [](uint16_t packet, bool success)
    {
        if(success){
            printf("packet succesfully sent %d", packet);
        }else{
            printf("packet failed to send %d", packet);
        }
    };

    printf("hello there");
    MAC::initialize(1, 2);

    //MAC::initialize(2, 2);
    MAC::getInstance()->setRXCallback(dataCallback);
    //LCMM::initialize(dataCallback, ackCallback);
    sleep_ms(1500);

    while (true)
    {
        //MAC::getInstance()->sendData(1, (unsigned char *)"Hello World!", strlen("Hello World!"), false, 5000);
        //LCMM::getInstance()->sendPacketSingle(true, 2, (unsigned char *)"Hello World.!", strlen("Hello World!"), ackCallback, 6000, 2);
        //sleep_ms(16000);
        sleep_ms(16);
        //printf("after sending packet \n");
        

        tight_loop_contents();
    }
    return 0;
}
