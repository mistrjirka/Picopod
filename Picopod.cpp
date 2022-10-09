#include <stdio.h>
#include "pico/stdlib.h"
#include <cstdlib>
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

void lol(RecievedPacket nice)
{
    printf("%d", nice.type);
}

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    Voltage.initVoltage(26, 2);

    LoraMessenger.LORASetup(lol);
    int result[15];
    char *message;
    while (true)
    {
        char startCommands = getchar_timeout_us(0);
        if (startCommands == 's' && !LoraMessengerClass::sending == true)
        {
            printf("\nlistening for commands\n");
            char *pLine = getLine(true, '\r');
            printf("\n%s\n", pLine);
            if (*pLine == *"pair" && strlen(pLine) == 6)
            {
                int id = (int)pLine[5] - 48;
                printf("\n%c\n", id);

                printf("sending packet\n");
                Packet packet;
                packet.target = id;
                packet.channel = LoraMessengerClass::current_channel;
                packet.type = COMMUNICATION_PAIRING;
                packet.content = "";
                packet.incomingType = COMMUNICATION_APPROVED;
                LoraMessengerClass::LORASendPacket(packet);
                //test();
                //LoraSendPacketLBT();
                //LoraMessenger.LoraSendPairingRequest(2, LoraMessengerClass::current_channel);
            }
            if (*pLine == *"send" && strlen(pLine) == 6)
            {
                int id = (int)pLine[5] - 48;
                free(pLine);
                printf("type the message then press enter: ");

                message = getLine(true, '\r');

                if (strlen(message) > 0)
                {
                    Packet packet;
                    packet.target = id;
                    packet.channel = LoraMessengerClass::current_channel;
                    packet.type = COMMUNICATION_STRING_MESSAGE;
                    printf("%s\n", message);
                    packet.content = strdup(message);
                    free(message);

                    printf("%s\n", message);
                    printf("%s\n", packet.content);
                    printf("%s\n", packet.content);
                    packet.incomingType = COMMUNICATION_OK_MESSAGE;
                    LoraMessengerClass::LORASendPacket(packet);
                }
            }

            tight_loop_contents();
        }
        tight_loop_contents();
    }
    return 0;
}
