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
// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
/*#define FLASH_TARGET_OFFSET (256 * 1024)

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE +
FLASH_TARGET_OFFSET);
*/
/*
bool sendTelemetry(struct repeating_timer *t)
{
    float packetloss;
    if ((lost_packets + sent_packets) != 0)
    {
        packetloss = (float(lost_packets) / float(sent_packets + lost_packets)) * 100;
    }
    else
    {
        packetloss = 2;
    }
    printf(("\n send telemetry packetloss:  " + std::to_string(packetloss) + " sent packets: " + std::to_string(sent_packets) + "\n lost packets: " + std::to_string(lost_packets) + "\n").c_str());
    BluetoothSend("\n packetloss:  " + std::to_string(packetloss) + " sent packets: " + std::to_string(sent_packets) + "\n lost packets: " + std::to_string(lost_packets) + "\n");
    if (ready_to_send_telemetry == true)
    {
        voltage = Voltage.getVoltage();
        message = "\n voltages: " + std::to_string(voltage);
        LORASendMessage(message, STRING_TELEMETRY_MESSAGE, true, false);
    }
    else
    {
        printf("lol no\n");
    }

    return true;
}
*/

void lol(Packet nice)
{
    printf("%d", nice.incomingType);
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
    /*LORASetup();
    BluetoothSetup();*/

    /* printf("settingup bluetooth");

    if (send_telemtery)
    {
        struct repeating_timer telemetry_timer;
        add_repeating_timer_ms(telemetry_update, sendTelemetry, NULL, &telemetry_timer);
    }*/

    // LoRa.onReceive(LORARecieveCallback);
    LoraMessenger.LORASetup(lol);

    int result[15];

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
            //test();
            // LoraSendPacketLBT();
            //  LoraMessenger.LoraSendPairingRequest(2, LoraMessengerClass::current_channel);
        }
        tight_loop_contents();
    }
    return 0;
}
