#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "lib/voltage/voltage.h"
#include "lib/lora/LoRa-RP2040.h"
#include <hardware/flash.h>

#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS 13
#define PIN_SCK 10
#define PIN_MOSI 11
#define CONFIGSET_OFFSET 0
#define CONFIGID_OFFSET 1
#define CONFIGTARGET_OFFSET 2
#define CONFIG_SIZE 3

#define ERROR_NO_MESSAGE 0
#define ERROR_INVALID_RECIPIENT 1
#define OK_MESSAGE 2
#define STRING_MESSAGE 3
#define STRING_TELEMETRY_MESSAGE 2


/*bluetooth modul*/
#define UART_ID                uart0
#define UART_BAUD_RATE         9600
#define UART_PIN_TX            0
#define UART_PIN_RX            1
#define DATA_BITS              8
#define STOP_BITS              1
#define PARITY                 UART_PARITY_NONE

const uint LED_PIN = 25;

bool send_telemtery = false;
int ID = 1;
int target_device = 2;

int spreading_factor = 11;
int power_level = 20;
int bandwidth = 41.7E3;

int time_before_confirmation_failed_ms = 600;
int attempts_before_failing = 3;
uint8_t msgCount = 0; // count of outgoing messages

int RSSI = 0;
int SNR = 0;
string recieved_message = "";


// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
/*#define FLASH_TARGET_OFFSET (256 * 1024)

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE +
FLASH_TARGET_OFFSET);
*/

void LORAsetup(void)
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
}
void LORAchangePowerLevel(int powerLevel)
{
    LoRa.setTxPower(powerLevel);
}
void LORAchangeSpreadingFactor(int spreadingFactor)
{
    LoRa.setSpreadingFactor(spreadingFactor);
}

void LORASendOKMessage(){
    printf("sending OK packet");
    LoRa.beginPacket();                    // start packet
    LoRa.write(target_device);             // add destination address
    LoRa.write(ID);                        // add sender address
    LoRa.write(msgCount);                  // add message ID
    LoRa.write(sizeof("ok") + 1); // add payload length
    LoRa.write(OK_MESSAGE);                   // add message type
    LoRa.write(0);                   // add message confirmation
   
    LoRa.print("ok");             //                 // add payload
    LoRa.endPacket();  
}
int LORAReceive()
{
    if (LoRa.parsePacket() == 0)
        return ERROR_NO_MESSAGE; // if there's no packet, return

    gpio_put(LED_PIN, 1);

    // read packet header uint8_ts:
    int recipient = LoRa.read(); // recipient address
    uint8_t sender = LoRa.read();         // sender address
    uint8_t incomingMsgId = LoRa.read();  // incoming msg ID
    uint8_t incomingLength = LoRa.read(); // incoming msg length
    uint8_t incomingType = LoRa.read(); //incoming msg type
    bool confirmation = LoRa.read();

    string incoming = "";

    while (LoRa.available())
    {
        incoming += (char)LoRa.read();
    }
    if (recipient != ID && recipient != 0)
    {
        printf("This message is not for me.\n");
        gpio_put(LED_PIN, 0);
        return ERROR_NO_MESSAGE; // if there's no packet,'skip rest of function
    }
    if(confirmation){
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
    printf(("Message content: " + incoming +"\n").c_str());
    printf("RSSI: %d\n",RSSI);
    printf("Snr: %d\n",SNR);
    
    gpio_put(LED_PIN, 0);
    return incomingType;
}
bool LORASendMessage(string message, int8_t message_type, bool confirmation, bool ignore_trafic)
{
    printf("sending message type %d, configrmation %d\n", message_type, confirmation);
    int failed_attempts = 0;
    bool sent = false;
    int random_break = LoRa.random();
    while (!sent)
    {
        printf("failed attempts %d\n", failed_attempts);
        if (failed_attempts > attempts_before_failing)
        {
            printf("Failed to send a message, too much RF traffic.\n");
            break;
        }
        if (LoRa.parsePacket() == 0 && ignore_trafic == false)
        {
            
                printf("rf sending\n");
                int n = message.length();
                char messageToSend[n + 1];
                strcpy(messageToSend, message.c_str());
                LoRa.beginPacket();                    // start packet
                LoRa.write(target_device);             // add destination address
                LoRa.write(ID);                        // add sender address
                LoRa.write(msgCount);                  // add message ID
                LoRa.write(sizeof(messageToSend) + 1); // add payload length

                LoRa.write(message_type);                   // add message type
                LoRa.write(confirmation);                   // add message confirmation
                LoRa.print(messageToSend);             //                 // add payload
                LoRa.endPacket();                      // finish packet and send it

                if (confirmation)
                {
                    printf("confirmation true\n");
                    int RecieveType = 0;
                    for (int i = 0; time_before_confirmation_failed_ms > i; i++)
                    {
                        
                        RecieveType = LORAReceive();
                        if(RecieveType == OK_MESSAGE){
                            printf("recieved OK message\n");
                            sent = true;
                            break;
                        }
                        sleep_ms(1);
                    }
                    if(sent){
                        printf("success_breaking\n");
                       break; 
                    }
                    printf("notsuccesfull breaking\n");
                }else{
                    printf("confirmation not true\n");
                    sent = true;
                    break;
                }
                failed_attempts++;
                printf("random Break %d", random_break*20);
                sleep_ms(random_break*25);
                random_break = LoRa.random();
        }
        else
        {
            failed_attempts++;
            LORAReceive();
        }
    }
    return sent;
}
int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    Voltage.initVoltage(26, 2);
    LORAsetup();
    int random_break = LoRa.random();
    /* if (!LoRa.begin(433.05E6)) {  }
    Voltage.initVoltage(26,2);
     if(flash_target_contents[FLASH_TARGET_OFFSET+CONFIGSET_OFFSET] != 0xf5){

            flash_range_erase(FLASH_TARGET_OFFSET, CONFIG_SIZE);
            uint8_t id = 0;
            uint8_t target = 0;
            printf("\nPlease input this device ID:");
           id = getchar();
            /*printf("\nOK");
            printf("\nPlease enter the target device ID:");
            target = getchar();
            printf("\nOK \n SAVING...\n");
            uint8_t data[CONFIG_SIZE];
            data[CONFIGSET_OFFSET] = 0xf5;
            data[CONFIGID_OFFSET] = id;
            data[CONFIGTARGET_OFFSET] = target;
            flash_range_erase(FLASH_TARGET_OFFSET, CONFIG_SIZE);
            flash_range_program(FLASH_TARGET_OFFSET, data, CONFIG_SIZE);
            printf("\nSAVED\n");

        }*/
    printf("lol");
    string message;
    float voltage;
    while (true)
    {

        if(send_telemtery){
                voltage =  Voltage.getVoltage();
                message = "\n voltages: " + std::to_string(voltage);
                printf("lol");
                LORASendMessage(message, STRING_TELEMETRY_MESSAGE, true, false);
               
                sleep_ms(random_break*55);
                random_break = LoRa.random();
        }
        LORAReceive();
    }
    return 0;
}
