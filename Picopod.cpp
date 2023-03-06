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
#include "lib/loramessenger/loramessenger.h"
#include "lib/IOUSBBT/IOUSBBT.h"
#include <hardware/flash.h>
#include <iostream>
#include <algorithm>

void lol(RecievedPacket packet)
{
    printf("{\"type\": %d, \"target\": %d, \"id\": %d, \"content\": \"%s\", \"rssi\":%d, \"snr\":%d, \"sender\": %d}\n", packet.type, packet.target, packet.id, packet.content.c_str(), packet.RSSI, packet.SNR, packet.sender);
    // std::cout << "{\"type\":" << packet.type << ",\"target\":" << packet.target << ",\"id\"" << packet.id << ",\"content\"" << packet.content << ",\"rssi\":" << packet.RSSI << ",\"SNR\":" << packet.SNR << "}";
}

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

Packet load_packet_information()
{
    Packet packet;
    packet.type = getchar() - '0';
    packet.incomingType = getchar() - '0';
    packet.target = getchar() - '0';
    packet.id = getchar() - '0';
    packet.channel = getchar() - '0';

    getMessage(&(packet.content));
    packet.content.erase(remove(packet.content.begin(), packet.content.end(), '\r'), packet.content.end());
    return packet;
}

void setup()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    Voltage.initVoltage(26, 2);

    LoraMessenger.LORASetup(lol);
}

void listenForCommands()
{
    char startCommands = getchar_timeout_us(0);
    if (startCommands == 's')
    {
        Packet packet = load_packet_information();
        if (packet.channel >= 0 && packet.channel <= 20)
        {
            LoraMessengerClass::LORAAddPacketToQueue(packet);
            printf("sent");
        }
        else
        {
            printf("{\"type\":-1}");
        }

        tight_loop_contents();
    }else if(startCommands == 'i'){
        printf("{\"type\":-1, \"deviceId:\": %d}", LoraMessengerClass::ID);
    }else if(startCommands == 'y'){
        int id = getchar_timeout_us(1000);
        if(id != PICO_ERROR_TIMEOUT){
            LoraMessengerClass::ID = id - '0';
        }
    }

}

int main()
{
    setup();
    std::cin.sync();

    while (true)
    {

        listenForCommands();
        tight_loop_contents();
    }
    return 0;
}
