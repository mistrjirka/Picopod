#include <Arduino.h>
#include <SPI.h>
#include <mac.h>
#include <RadioLib.h>
// put function declarations here:

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9

#define SPI1_MISO 12
#define SPI1_MOSI 11
#define SPI1_SCLK 10
arduino::MbedSPI spiint = MbedSPI(SPI1_MISO, SPI1_MOSI, SPI1_SCLK);
SPISettings spiSettings(200000, MSBFIRST, SPI_MODE0);
LLCC68 radio = new Module(13, 9, 14, 19, spiint, spiSettings);

MAC::PacketReceivedCallback dataCallback = [](MACPacket *packet, uint16_t size, uint32_t crcCalculated)
{
  // Perform actions with the received packet and size
  // For example, print the packet data to the console
  Serial.print("Received packet from " + String(packet->sender) + " to " + String(packet->target) + " with packet type: \n");
  for (int i = 0; i < size; i++)
  {
    printf("%c \n", packet->data[i]);
  }
  printf("\n");
  if (packet)
  {
    free(packet);
    packet = NULL;
  }
};

void setup()
{

  spiint.begin();
  Serial.begin(115200);
  delay(5000);
  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin(433.30, 125.0, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8, 0);

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
  //MAC::initialize(radio, 1, 2);
  MAC::initialize(
    radio, 
    2, 
    2, 
    9, 
    125.0, 
    15, 
    22, 
    7);
  //MAC::getInstance()->setRXCallback(dataCallback);
  Serial.print(F("After init"));

  // some modules have an external RF switch
  // controlled via two pins (RX enable, TX enable)
  // to enable automatic control of the switch,
  // call the following method
  // RX enable:   4
  // TX enable:   5
  /*
    radio.setRfSwitchPins(4, 5);
  */
}


int count = 0;

void loop() {  
  MAC::getInstance()->loop();
  if(count++%200 == 0)
  MAC::getInstance()->sendData(2, (unsigned char *)"hello there general kenobi shit sda asdasd asd sad sad  sadasd asd asd fdsfdas asdasd asd asdasd asd asd", strlen("hello there general kenobi shit sda asdasd asd sad sad  sadasd asd asd fdsfdas asdasd asd asdasd asd asd")+1, false);
  // wait for a second before transmitting again
  delay(50);
}