#include <Arduino.h>
#include <SPI.h>
#include <mac.h>
#include <lcmm.h>
#include <DTPK.h>
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
/*
LCMM::DataReceivedCallback dataCallback = [](LCMMPacketDataRecieve *packet, uint32_t size)
{
  // Perform actions with the received packet and size
  // For example, print the packet data to the console
  Serial.println("Received packet from " + String(packet->mac.sender) + " to " + String(packet->mac.target) + " with packet type: " + String(packet->type) + ": \n");
  for (unsigned int i = 0; i < (int)size-sizeof(LCMMPacketDataRecieve); i++)
  {
    Serial.println(packet->data[i] );
  }
  Serial.println();
  if (packet)
  {
    free(packet);
    packet = NULL;
  }
};
LCMM::AcknowledgmentCallback ackCallback = [](uint16_t packet, bool success)
{
  if (success)
  {
    Serial.println("packet succesfully sent " + packet);
  }
  else
  {
    Serial.println("packet failed to send "+ packet);
  }
};*/
/*
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
*/

DTPK::PacketReceivedCallback dataCallback(DTPKPacketGenericReceive *packet, size_t size)
{
  printf("packetdata: %s", packet->data);
}
void setup()
{

  spiint.begin();
  Serial.begin(115200);
  delay(5000);
  // initialize SX1262 with default settingsRADIOLIB_SX126X_SYNC_WORD_PRIVATE
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
  // MAC::initialize(radio, 1, 2);
  uint16_t id = 4;
  uint8_t NAPInterval = 20;
  MAC::initialize(
      radio,
      id,
      2,
      9,
      125.0,
      15,
      22,
      7);
  DTPK::initialize(NAPInterval);
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

void loop()
{
  static int count = 0;
  DTPK::getInstance()->loop();
  if (count++ % 10000 == 0){
    vector<NeighborRecord> neighbors = DTPK::getInstance()->getNeighbours();

    for (int i = 0; i < neighbors.size(); i++)
    {
      printf("Neighbor %d: id: %d, distance: %d, from: %d\n", i, neighbors[i].id, neighbors[i].distance, neighbors[i].from);
      Serial.println("Neighbor " + String(i) + ": target: " + String(neighbors[i].id) + ", distance: " + String(neighbors[i].distance) + ", through: " + String(neighbors[i].from) + "\n");
    }
    //LCMM::getInstance()->sendPacketSingle(true, 2, (unsigned char *)"hello there general kenobi shit sda", strlen("hello there general kenobi shit sda") + 1, ackCallback);
    //MAC::getInstance()->sendData(2, (unsigned char *)"hello there general kenobi shit sda", strlen("hello there general kenobi shit sda") + 1, false);
  }
  // wait for a second before transmitting again
  delay(1);
}