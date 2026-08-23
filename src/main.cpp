#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include <SPI.h>

#include <DTPK.h>
#include <mac.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <vector>

#ifndef PICOPOD_NODE_ID
#define PICOPOD_NODE_ID 6
#endif

namespace
{
constexpr uint16_t NODE_ID = PICOPOD_NODE_ID;
constexpr uint8_t RADIO_CHANNEL = 2;
constexpr uint8_t RADIO_SF = 8;
constexpr float RADIO_BANDWIDTH_KHZ = 125.0f;
constexpr int8_t RADIO_SQUELCH_DB = 15;
constexpr int8_t RADIO_POWER_DBM = 22;
constexpr uint8_t RADIO_CODING_RATE = 7;
constexpr float RADIO_FREQUENCY_MHZ = 433.425f;
constexpr uint16_t RADIO_PREAMBLE = 8;
constexpr float RADIO_TCXO_VOLTAGE = 0.0f;

// Original custom-board wiring recovered from the historical firmware.
constexpr pin_size_t RADIO_MISO = 12;
constexpr pin_size_t RADIO_MOSI = 11;
constexpr pin_size_t RADIO_SCK = 10;
constexpr pin_size_t RADIO_CS = 13;
constexpr pin_size_t RADIO_DIO1 = 9;
constexpr pin_size_t RADIO_RESET = 14;
constexpr pin_size_t RADIO_BUSY = 19;
constexpr pin_size_t STATUS_LED = 25;

constexpr uint32_t BOOT_MAGIC = 0x44545033u; // "DTP3"
constexpr size_t EEPROM_BYTES = 256;
constexpr size_t SERIAL_LINE_BYTES = 256;

struct BootRecord
{
    uint32_t magic;
    uint16_t sequence;
    uint16_t inverse;
};

SPISettings radioSpiSettings(200000, MSBFIRST, SPI_MODE0);
LLCC68 radio = new Module(
    RADIO_CS,
    RADIO_DIO1,
    RADIO_RESET,
    RADIO_BUSY,
    SPI1,
    radioSpiSettings);

char serialLine[SERIAL_LINE_BYTES]{};
size_t serialLineLength = 0;

uint16_t randomNonzeroSequence()
{
    uint16_t value = static_cast<uint16_t>(rp2040.hwrand32());
    return value == 0 ? 1 : value;
}

uint16_t nextBootSequence()
{
    EEPROM.begin(EEPROM_BYTES);

    BootRecord record{};
    EEPROM.get(0, record);
    const bool valid =
        record.magic == BOOT_MAGIC &&
        record.sequence != 0 &&
        record.inverse == static_cast<uint16_t>(~record.sequence);

    uint16_t sequence = valid
                            ? static_cast<uint16_t>(record.sequence + 1u)
                            : randomNonzeroSequence();
    if (sequence == 0)
        sequence = 1;

    BootRecord updated{
        BOOT_MAGIC,
        sequence,
        static_cast<uint16_t>(~sequence)};
    EEPROM.put(0, updated);
    if (!EEPROM.commit())
        Serial.println("Warning: boot sequence could not be persisted");
    EEPROM.end();
    return sequence;
}

void printPayload(const uint8_t *payload, size_t size)
{
    for (size_t index = 0; index < size; ++index)
    {
        const uint8_t value = payload[index];
        Serial.write(value >= 32 && value < 127 ? value : '.');
    }
}

void receivedDTPKPacket(DTPKPacketGeneric *packet, uint16_t size)
{
    if (!packet || size < sizeof(DTPKPacketGeneric))
        return;
    const size_t payloadSize = size - sizeof(DTPKPacketGeneric);
    Serial.printf("RX from node %u (%u bytes): ",
                  packet->originalSender,
                  static_cast<unsigned>(payloadSize));
    printPayload(packet->data, payloadSize);
    Serial.println();
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
}

void printRoutes()
{
    DTPK *dtpk = DTPK::getInstance();
    if (!dtpk)
    {
        Serial.println("DTProtocol is not initialized");
        return;
    }
    const std::vector<NeighborRecord> routes = dtpk->getNeighbours();
    Serial.printf("Routes: %u\n", static_cast<unsigned>(routes.size()));
    for (const NeighborRecord &route : routes)
    {
        Serial.printf("  node=%u via=%u hops=%u\n",
                      route.id, route.from, route.distance);
    }
}

void printStatus()
{
    MAC *mac = MAC::getInstance();
    if (!mac)
    {
        Serial.println("MAC is not initialized");
        return;
    }
    const MAC::Diagnostics &diagnostics = mac->getDiagnostics();
    Serial.printf(
        "node=%u radio=%s error=%d frequency=%.3f MHz SF%u BW%.0f\n"
        "radio_errors=%lu rx_read=%lu rx_short=%lu rx_alloc=%lu heap=%d\n",
        NODE_ID,
        mac->isReady() ? "ready" : "fault",
        mac->getLastRadioError(),
        RADIO_FREQUENCY_MHZ,
        RADIO_SF,
        RADIO_BANDWIDTH_KHZ,
        static_cast<unsigned long>(diagnostics.radioCommandErrors),
        static_cast<unsigned long>(diagnostics.rxReadErrors),
        static_cast<unsigned long>(diagnostics.rxTooShort),
        static_cast<unsigned long>(diagnostics.rxAllocationFailures),
        rp2040.getFreeHeap());
}

void sendText(uint16_t target, const char *text)
{
    if (!text || !*text || !DTPK::getInstance())
        return;
    const uint16_t id = DTPK::getInstance()->sendPacket(
        target,
        reinterpret_cast<unsigned char *>(const_cast<char *>(text)),
        strlen(text),
        60000,
        true,
        [target](uint8_t success, uint16_t ping) {
            Serial.printf("TX to %u: %s (%u ms)\n",
                          target,
                          success ? "delivered" : "failed",
                          ping);
        });
    if (id == 0)
        Serial.println("TX rejected: no route, invalid size, or protocol busy");
    else
        Serial.printf("TX queued: packet %u to node %u\n", id, target);
}

void printHelp()
{
    Serial.println(
        "Commands:\n"
        "  status\n"
        "  routes\n"
        "  send <node> <text>\n"
        "  reboot\n"
        "  help");
}

void handleCommand(char *line)
{
    while (*line && std::isspace(static_cast<unsigned char>(*line)))
        ++line;
    if (!*line)
        return;

    if (strcmp(line, "status") == 0)
        printStatus();
    else if (strcmp(line, "routes") == 0)
        printRoutes();
    else if (strcmp(line, "help") == 0)
        printHelp();
    else if (strcmp(line, "reboot") == 0)
        rp2040.reboot();
    else if (strncmp(line, "send ", 5) == 0)
    {
        char *targetText = line + 5;
        char *end = nullptr;
        const unsigned long parsed = strtoul(targetText, &end, 10);
        while (end && *end && std::isspace(static_cast<unsigned char>(*end)))
            ++end;
        if (parsed == 0 || parsed > UINT16_MAX || !end || !*end)
            Serial.println("Usage: send <node 1..65535> <text>");
        else
            sendText(static_cast<uint16_t>(parsed), end);
    }
    else
        printHelp();
}

void serviceSerial()
{
    while (Serial.available())
    {
        const char value = static_cast<char>(Serial.read());
        if (value == '\r')
            continue;
        if (value == '\n')
        {
            serialLine[serialLineLength] = '\0';
            handleCommand(serialLine);
            serialLineLength = 0;
            continue;
        }
        if (serialLineLength + 1u < sizeof(serialLine))
            serialLine[serialLineLength++] = value;
        else
        {
            serialLineLength = 0;
            Serial.println("Command discarded: line too long");
        }
    }
}

bool initializeRadio()
{
    for (uint8_t attempt = 0; attempt < 3; ++attempt)
    {
        const int16_t state = radio.begin(
            RADIO_FREQUENCY_MHZ,
            RADIO_BANDWIDTH_KHZ,
            RADIO_SF,
            RADIO_CODING_RATE,
            RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
            RADIO_POWER_DBM,
            RADIO_PREAMBLE,
            RADIO_TCXO_VOLTAGE);
        if (state == RADIOLIB_ERR_NONE)
            return true;
        Serial.printf("LLCC68 initialization failed: %d (attempt %u)\n",
                      state, attempt + 1u);
        delay(150);
    }
    return false;
}

[[noreturn]] void restartAfterFatal(const char *reason)
{
    Serial.println(reason);
    Serial.println("Restarting in 5 seconds");
    Serial.flush();
    delay(5000);
    rp2040.reboot();
    while (true)
        delay(1000);
}
} // namespace

void setup()
{
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 1500u;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0)
        delay(10);

    SPI1.setRX(RADIO_MISO);
    SPI1.setTX(RADIO_MOSI);
    SPI1.setSCK(RADIO_SCK);
    SPI1.setCS(RADIO_CS);
    SPI1.begin();

    if (!initializeRadio())
        restartAfterFatal("LLCC68 radio could not be initialized");
    if (!MAC::initialize(
            radio,
            NODE_ID,
            MACRegion::EU433,
            RADIO_CHANNEL,
            RADIO_SF,
            RADIO_BANDWIDTH_KHZ,
            RADIO_SQUELCH_DB,
            RADIO_POWER_DBM,
            RADIO_CODING_RATE))
        restartAfterFatal("MAC initialization failed");
    if (!DTPK::initialize(20, nextBootSequence(), false))
        restartAfterFatal("DTProtocol initialization failed");

    DTPK::getInstance()->setPacketReceivedCallback(receivedDTPKPacket);
    digitalWrite(STATUS_LED, HIGH);
    Serial.printf(
        "Picopod RP2040 node %u ready on %.3f MHz, SF%u\n",
        NODE_ID, RADIO_FREQUENCY_MHZ, RADIO_SF);
    printHelp();
}

void loop()
{
    DTPK::getInstance()->loop();
    serviceSerial();

    static uint32_t lastStatus = 0;
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastStatus) >= 10000u)
    {
        lastStatus = now;
        printStatus();
    }
    delay(1);
}
