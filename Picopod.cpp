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
#include <hardware/flash.h>
#define L 20
unsigned char str[L + 1];

void lol(RecievedPacket nice)
{
    printf("%d", nice.type);
}

const uint startLineLength = 8; // the linebuffer will automatically grow for longer lines
const char eof = 255;           // EOF in stdio.h -is -1, but getchar returns int 255 to avoid blocking

/*
 *  read a line of any  length from stdio (grows)
 *
 *  @param fullDuplex input will echo on entry (terminal mode) when false
 *  @param linebreak defaults to "\n", but "\r" may be needed for terminals
 *  @return entered line on heap - don't forget calling free() to get memory back
 */
static char *getLine(bool fullDuplex = true, char lineBreak = '\n')
{
    // th line buffer
    // will allocated by pico_malloc module if <cstdlib> gets included
    char *pStart = (char *)malloc(startLineLength);
    char *pPos = pStart;             // next character position
    size_t maxLen = startLineLength; // current max buffer size
    size_t len = maxLen;             // current max length
    int c;

    if (!pStart)
    {
        return NULL; // out of memory or dysfunctional heap
    }

    while (1)
    {
        c = getchar(); // expect next character entry
        if (c == eof || c == lineBreak)
        {
            break; // non blocking exit
        }

        if (fullDuplex)
        {
            putchar(c); // echo for fullDuplex terminals
        }

        if (--len == 0)
        { // allow larger buffer
            len = maxLen;
            // double the current line buffer size
            char *pNew = (char *)realloc(pStart, maxLen *= 2);
            if (!pNew)
            {
                free(pStart);
                return NULL; // out of memory abort
            }
            // fix pointer for new buffer
            pPos = pNew + (pPos - pStart);
            pStart = pNew;
        }

        // stop reading if lineBreak character entered
        if ((*pPos++ = c) == lineBreak)
        {
            break;
        }
        tight_loop_contents();
    }

    *pPos = '\0'; // set string end mark
    return pStart;
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
                // test();
                //  LoraSendPacketLBT();
                //   LoraMessenger.LoraSendPairingRequest(2, LoraMessengerClass::current_channel);
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
