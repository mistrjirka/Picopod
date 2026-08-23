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
constexpr uint8_t RADIO_CHANNEL = 0;
constexpr uint8_t RADIO_SF = 9;
constexpr float RADIO_BANDWIDTH_KHZ = 125.0f;
constexpr int8_t RADIO_SQUELCH_DB = 15;
constexpr int8_t RADIO_POWER_DBM = 13;
constexpr uint8_t RADIO_CODING_RATE = 7;
constexpr uint16_t RADIO_PREAMBLE = 8;

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

bool initializeRadio()
{
    // Heltec revisions are normally populated with a DIO3-powered 1.8 V TCXO.
    // Retry with an externally powered XTAL setting as a safe board-revision
    // fallback instead of hanging forever on a single assumption.
    constexpr float oscillatorModes[] = {RADIO_TCXO_VOLTAGE, 0.0f};
    int16_t lastError = RADIOLIB_ERR_NONE;
    for (float tcxoVoltage : oscillatorModes)
    {
        for (uint8_t attempt = 0; attempt < 2; ++attempt)
        {
            lastError = radio.begin(
                868.100f,
                RADIO_BANDWIDTH_KHZ,
                RADIO_SF,
                RADIO_CODING_RATE,
                RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                RADIO_POWER_DBM,
                RADIO_PREAMBLE,
                tcxoVoltage,
                false);
            if (lastError == RADIOLIB_ERR_NONE)
            {
                Serial.printf("SX1262 ready (TCXO %.1f V)\n", tcxoVoltage);
                return true;
            }
            Serial.printf(
                "SX1262 start failed: error=%d tcxo=%.1f attempt=%u\n",
                lastError, tcxoVoltage, attempt + 1u);
            delay(150);
        }
    }
    return false;
}

[[noreturn]] void restartAfterFatal(const char *reason)
{
    Serial.println(reason);
    Serial.println("Restarting in 5 seconds");
    Serial.flush();
    delay(5000);
    ESP.restart();
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
    if (!initializeRadio())
        restartAfterFatal("SX1262 initialization failed");

    if (!MAC::initialize(
        radio,
        NODE_ID,
        MACRegion::EU868,
        RADIO_CHANNEL,
        RADIO_SF,
        RADIO_BANDWIDTH_KHZ,
        RADIO_SQUELCH_DB,
        RADIO_POWER_DBM,
        RADIO_CODING_RATE))
        restartAfterFatal("MAC initialization failed");
    if (!DTPK::initialize(20, nextBootSequence(), false))
        restartAfterFatal("DTProtocol initialization failed");

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
        MAC *mac = MAC::getInstance();
        const BluetoothDiagnostics bleDiagnostics =
            Bluetooth::getInstance()->getDiagnostics();
        Serial.printf(
            "Routes: %u  radio=%s error=%d  BLE=%s MTU=%u drops=%lu/%lu/%lu\n",
            static_cast<unsigned>(neighbors.size()),
            mac && mac->isReady() ? "ready" : "fault",
            mac ? mac->getLastRadioError() : INT16_MIN,
            Bluetooth::getInstance()->deviceIsConnected()
                ? "connected"
                : "advertising",
            Bluetooth::getInstance()->negotiatedMtu(),
            static_cast<unsigned long>(bleDiagnostics.droppedWrites),
            static_cast<unsigned long>(bleDiagnostics.droppedInboundMessages),
            static_cast<unsigned long>(bleDiagnostics.droppedControlNotifications));
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
