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
    std::cin >> packet.type;
    std::cin >> packet.incomingType;
    std::cin >> packet.target;
    std::cin >> packet.id;
    std::cin >> packet.channel;

    getMessage(&(packet.content));
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
            LoraMessengerClass::LORASendPacket(packet);
        }
        else
        {
            printf("{\"type\":-1}");
        }

        tight_loop_contents();
    }
}

int main()
{
    setup();
    while (true)
    {
        listenForCommands();
        tight_loop_contents();
    }
    return 0;
}
