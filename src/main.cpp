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

// counter to keep track of transmitted packets
/*
void loop()
{
  //MAC::getInstance()->loop();
  // you can transmit C-string or Arduino string up to
  // 256 characters long
  /*MAC::getInstance()->sendData(2, (unsigned char *)"hello there general kenobi shit", strlen("hello there general kenobi shit")+1, false);
  delay(10000);
  // wait for a second before transmitting again
  //delay(30);*//*
}
*/
int count = 0;

void loop() {
  Serial.println(F("[SX1262] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  //String str = "Hello World!sda dasadsdas as dasd  asdsd aasd asd asd d sadsa  # " + String(count++);
  //int state = radio.transmit(str);

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */
/*
  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }*/
  MAC::getInstance()->loop();
  MAC::getInstance()->sendData(2, (unsigned char *)"hello there general kenobi shit sda asdasd asd sad sad  sadasd asd asd fdsfdas asdasd asd asdasd asd asd", strlen("hello there general kenobi shit sda asdasd asd sad sad  sadasd asd asd fdsfdas asdasd asd asdasd asd asd")+1, false);
  // wait for a second before transmitting again
  delay(2000);
}