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

void lol(RecievedPacket nice)
{
    printf("%d", nice.type);
    if (nice.type == COMMUNICATION_STRING_MESSAGE)
    {
        printf("\n\n MESSAGE: \n\n%s", nice.content.c_str());
    }
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
    std::getline(std::cin, *message, '.');
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

void listenForCommands(std::string *command, int *id, std::string *message)
{
    char startCommands = getchar_timeout_us(0);
    if (startCommands == 's' && !LoraMessengerClass::sending == true)
    {
        printf("\nlistening for commands\n");

        *command = "";
        *id = -1;

        getCommand(command, id);
        printf("%s %d", (*command).c_str(), *id);
        printf("%d", LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, *id));

        if ((*command).find("pair") != -1 && LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, *id) == -1)
        {
            printf("\n%d\n", *id);
            printf("sending packet\n");
            Packet packet;
            packet.target = *id;
            packet.channel = LoraMessengerClass::current_channel;
            packet.type = COMMUNICATION_PAIRING;
            packet.content = "";
            packet.incomingType = COMMUNICATION_APPROVED;
            LoraMessengerClass::LORASendPacket(packet);
            //test();
            //LoraSendPacketLBT();
            //LoraMessenger.LoraSendPairingRequest(2, LoraMessengerClass::current_channel);
        }
        if ((*command).find("send") != -1 && LoraMessengerClass::searchAdressBook(LoraMessengerClass::addressBook, *id) != -1)
        {
            printf("type the message then press enter: ");

            getMessage(message);

            if ((*message).length())
            {
                Packet packet;
                packet.target = *id;
                packet.channel = LoraMessengerClass::current_channel;
                packet.type = COMMUNICATION_STRING_MESSAGE;
                packet.content = (*message).c_str();

                packet.incomingType = COMMUNICATION_OK_MESSAGE;
                LoraMessengerClass::LORASendPacket(packet);
            }
        }

        tight_loop_contents();
    }
}

int main()
{
    setup();
    std::string command = "";
    int id = -1;
    std::string message;
    while (true)
    {
        listenForCommands(&command, &id, &message);
        tight_loop_contents();
    }
    return 0;
}
