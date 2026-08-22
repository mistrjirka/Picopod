#include <Arduino.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <SPI.h>

#include <DTPK.h>
#include <bluetooth.h>
#include <mac.h>

namespace
{
constexpr uint16_t NODE_ID = 12;
constexpr int RADIO_CS = 8;
constexpr int RADIO_RESET = 12;
constexpr int RADIO_DIO1 = 14;
constexpr int RADIO_BUSY = 13;
constexpr int RADIO_MISO = 11;
constexpr int RADIO_MOSI = 10;
constexpr int RADIO_SCK = 9;
constexpr float RADIO_TCXO_VOLTAGE = 1.8f;

SPIClass radioBus(FSPI);
SX1262 radio = new Module(
    RADIO_CS,
    RADIO_DIO1,
    RADIO_RESET,
    RADIO_BUSY,
    radioBus);

uint16_t nextBootSequence()
{
    Preferences preferences;
    if (!preferences.begin("dtpk", false))
    {
        uint16_t fallback = static_cast<uint16_t>(esp_random());
        return fallback == 0 ? 1 : fallback;
    }
    uint16_t sequence = preferences.getUShort("boot-seq", 0);
    ++sequence;
    if (sequence == 0)
        sequence = 1;
    preferences.putUShort("boot-seq", sequence);
    preferences.end();
    return sequence;
}

void receivedDTPKPacket(DTPKPacketGeneric *packet, uint16_t size)
{
    if (!packet || size < sizeof(DTPKPacketGeneric))
        return;

    const size_t payloadSize = size - sizeof(DTPKPacketGeneric);
    Serial.printf(
        "DTPK message from %u (%u bytes): ",
        packet->originalSender,
        static_cast<unsigned>(payloadSize));
    for (size_t index = 0; index < payloadSize; ++index)
    {
        const char value = static_cast<char>(packet->data[index]);
        Serial.print(value >= 32 && value < 127 ? value : '.');
    }
    Serial.println();
}

[[noreturn]] void haltWithRadioError(int16_t state)
{
    Serial.printf("SX1262 initialization failed: %d\n", state);
    while (true)
        delay(1000);
}
} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    // Wireless Stick Lite V3 wiring: SCK9/MISO11/MOSI10, NSS8, RST12,
    // DIO1 14, BUSY13. DIO2 controls the on-board RF switch and DIO3 the
    // 1.8 V TCXO through RadioLib's SX1262 initialization.
    radioBus.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_CS);
    const int16_t state = radio.begin(
        868.100f,
        125.0f,
        9,
        7,
        RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
        13,
        8,
        RADIO_TCXO_VOLTAGE,
        false);
    if (state != RADIOLIB_ERR_NONE)
        haltWithRadioError(state);

    MAC::initialize(
        radio,
        NODE_ID,
        MACRegion::EU868,
        0,
        9,
        125.0f,
        15,
        13,
        7);
    DTPK::initialize(20, nextBootSequence(), false);

    Bluetooth::initialize();
    Bluetooth *bluetooth = Bluetooth::getInstance();
    bluetooth->setDeviceName("DTPK-StickLiteV3");
    bluetooth->setDTPKPacketCallback(receivedDTPKPacket);
    if (!bluetooth->setup())
        Serial.println("BLE initialization failed");

    Serial.printf("DTPK v3 node %u ready on EU868\n", NODE_ID);
}

void loop()
{
    Bluetooth::getInstance()->loop();

    static uint32_t lastPrint = 0;
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastPrint) >= 5000u)
    {
        lastPrint = now;
        const std::vector<NeighborRecord> neighbors =
            DTPK::getInstance()->getNeighbours();
        Serial.printf("Routes: %u\n", static_cast<unsigned>(neighbors.size()));
        for (const NeighborRecord &neighbor : neighbors)
        {
            Serial.printf(
                "  destination=%u next=%u distance=%u\n",
                neighbor.id,
                neighbor.from,
                neighbor.distance);
        }
    }
    delay(1);
}
