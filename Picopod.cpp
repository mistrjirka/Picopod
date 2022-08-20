#include <stdio.h>
#include "pico/stdlib.h"
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
#include <hardware/flash.h>
#define L 20
unsigned char str[L + 1];

void lol(RecievedPacket nice)
{
    printf("%d", nice.type);
}
unsigned char *readLine()
{

    unsigned char u, *p;
    for (p = str, u = getchar(); u != '\r' && p - str < L; u = getchar())
        putchar(*p++ = u);
    *p = 0;
    return str;
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
    /*Packet packet;
    packet.target = 2;
    packet.channel = LoraMessengerClass::current_channel;
    packet.type = COMMUNICATION_PAIRING;
   LoraMessengerClass::LORASendPacket(packet);*/

    while (true)
    {
        char ch = getchar_timeout_us(0);
        if (ch == 's')
        {
            Packet packet;
            packet.target = 2;
            packet.channel = LoraMessengerClass::current_channel;
            packet.type = COMMUNICATION_PAIRING;
            LoraMessengerClass::LORASendPacket(packet);
            // test();
            //  LoraSendPacketLBT();
            //   LoraMessenger.LoraSendPairingRequest(2, LoraMessengerClass::current_channel);
        }
        tight_loop_contents();
    }
    return 0;
}
