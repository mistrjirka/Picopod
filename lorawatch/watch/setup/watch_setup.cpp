#include "watch_setup.h"

#include <Arduino.h>
#include <Preferences.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <DTPK.h>
#include <LV_Helper.h>
#include <LilyGoLib.h>
#include <bluetooth.h>
#include <mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace
{
constexpr uint16_t NODE_ID = 3;
constexpr uint8_t RADIO_CHANNEL = 0;
constexpr uint8_t RADIO_SF = 9;
constexpr float RADIO_BANDWIDTH_KHZ = 125.0f;
constexpr int8_t RADIO_SQUELCH_DB = 15;
constexpr int8_t RADIO_POWER_DBM = 13;
constexpr uint8_t RADIO_CODING_RATE = 7;
constexpr uint32_t ROUTE_REFRESH_MS = 1000;
constexpr uint32_t SENSOR_REFRESH_MS = 1000;
constexpr uint32_t DIAGNOSTIC_REFRESH_MS = 2000;
constexpr uint32_t DISPLAY_OFF_SERVICE_MS = 50;
constexpr uint8_t SETTINGS_BRIGHTNESS_MIN = 5;
constexpr size_t MAX_UI_MESSAGE_BYTES = 112;
constexpr size_t MAX_TABLE_ROUTES = 32;

constexpr std::array<uint32_t, 5> SCREEN_TIMEOUTS = {
    30000u, 60000u, 120000u, 300000u, UINT32_MAX};
constexpr std::array<const char *, 5> QUICK_MESSAGES = {
    "Hello", "OK", "Need help", "Where are you?", "I am safe"};

SX1262 &radio = watch;

struct UiEvent
{
    enum class Type : uint8_t
    {
        Incoming,
        SendResult,
        System
    } type = Type::System;
    uint16_t sender = 0;
    uint16_t ping = 0;
    bool success = false;
    char text[MAX_UI_MESSAGE_BYTES]{};
};

struct AppSettings
{
    uint8_t brightness = 55;
    uint8_t timeoutIndex = 2;
    bool haptic = true;
};

QueueHandle_t uiQueue = nullptr;
std::atomic<bool> wakeRequested{false};
portMUX_TYPE irqMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool pmuIrq = false;
volatile bool motionIrq = false;

AppSettings settings;
bool displayOn = true;
bool usbConnected = false;
bool sending = false;
uint32_t lastRouteRefresh = 0;
uint32_t lastSensorRefresh = 0;
uint32_t lastDiagnosticRefresh = 0;
uint32_t lastDisplayService = 0;
bool settingsDirty = false;
uint32_t settingsSaveAt = 0;
uint32_t stepCount = 0;
uint16_t selectedTarget = UINT16_MAX;
uint8_t selectedQuickMessage = 0;
std::vector<NeighborRecord> routes;

lv_obj_t *tileview = nullptr;
lv_obj_t *homeTime = nullptr;
lv_obj_t *homeDate = nullptr;
lv_obj_t *homeStatus = nullptr;
lv_obj_t *homeMetrics = nullptr;
lv_obj_t *homeEvent = nullptr;
lv_obj_t *targetDropdown = nullptr;
lv_obj_t *quickMessageDropdown = nullptr;
lv_obj_t *sendButton = nullptr;
lv_obj_t *sendStatus = nullptr;
lv_obj_t *inboxLabel = nullptr;
lv_obj_t *meshSummary = nullptr;
lv_obj_t *meshTable = nullptr;
lv_obj_t *scanButton = nullptr;
lv_obj_t *scanStatus = nullptr;
lv_obj_t *brightnessSlider = nullptr;
lv_obj_t *brightnessValue = nullptr;
lv_obj_t *timeoutDropdown = nullptr;
lv_obj_t *hapticSwitch = nullptr;
lv_obj_t *diagnosticLabel = nullptr;

lv_style_t pageStyle;
lv_style_t cardStyle;
lv_style_t titleStyle;
lv_style_t primaryButtonStyle;

bool routeLess(const NeighborRecord &left, const NeighborRecord &right)
{
    if (left.id != right.id)
        return left.id < right.id;
    if (left.distance != right.distance)
        return left.distance < right.distance;
    return left.from < right.from;
}

bool routesEqual(
    const std::vector<NeighborRecord> &left,
    const std::vector<NeighborRecord> &right)
{
    if (left.size() != right.size())
        return false;
    for (size_t index = 0; index < left.size(); ++index)
    {
        if (left[index].id != right[index].id ||
            left[index].from != right[index].from ||
            left[index].distance != right[index].distance)
            return false;
    }
    return true;
}

uint16_t nextBootSequence()
{
    Preferences preferences;
    if (!preferences.begin("dtpk", false))
    {
        uint16_t fallback = static_cast<uint16_t>(esp_random());
        return fallback == 0 ? 1 : fallback;
    }
    uint16_t sequence = preferences.getUShort("boot-seq", 0);
    sequence = static_cast<uint16_t>(sequence + 1u);
    if (sequence == 0)
        sequence = 1;
    preferences.putUShort("boot-seq", sequence);
    preferences.end();
    return sequence;
}

void loadSettings()
{
    Preferences preferences;
    if (!preferences.begin("picopod", true))
        return;
    settings.brightness = std::max<uint8_t>(
        SETTINGS_BRIGHTNESS_MIN,
        std::min<uint8_t>(100, preferences.getUChar("brightness", 55)));
    settings.timeoutIndex = std::min<uint8_t>(
        SCREEN_TIMEOUTS.size() - 1,
        preferences.getUChar("timeout", 2));
    settings.haptic = preferences.getBool("haptic", true);
    preferences.end();
}

void saveSettings()
{
    Preferences preferences;
    if (!preferences.begin("picopod", false))
        return;
    preferences.putUChar("brightness", settings.brightness);
    preferences.putUChar("timeout", settings.timeoutIndex);
    preferences.putBool("haptic", settings.haptic);
    preferences.end();
}

void markSettingsDirty()
{
    settingsDirty = true;
    settingsSaveAt = millis() + 1500u;
}

void saveSettingsWhenDue(uint32_t now)
{
    if (!settingsDirty || static_cast<int32_t>(now - settingsSaveAt) < 0)
        return;
    saveSettings();
    settingsDirty = false;
}

void postUiEvent(const UiEvent &event)
{
    if (!uiQueue)
        return;
    if (xQueueSend(uiQueue, &event, 0) != pdTRUE)
    {
        UiEvent discarded;
        (void)xQueueReceive(uiQueue, &discarded, 0);
        (void)xQueueSend(uiQueue, &event, 0);
    }
    wakeRequested.store(true, std::memory_order_release);
}

void postSystemMessage(const char *text)
{
    UiEvent event;
    event.type = UiEvent::Type::System;
    snprintf(event.text, sizeof(event.text), "%s", text ? text : "");
    postUiEvent(event);
}

void onDTPKPacket(DTPKPacketGeneric *packet, uint16_t size)
{
    if (!packet || size < sizeof(DTPKPacketGeneric))
        return;
    UiEvent event;
    event.type = UiEvent::Type::Incoming;
    event.sender = packet->originalSender;
    const size_t payloadSize = size - sizeof(DTPKPacketGeneric);
    const size_t count = std::min(payloadSize, sizeof(event.text) - 1u);
    for (size_t index = 0; index < count; ++index)
    {
        const uint8_t value = packet->data[index];
        event.text[index] = value >= 32 && value < 127
                                ? static_cast<char>(value)
                                : '.';
    }
    event.text[count] = '\0';
    postUiEvent(event);
}

void onSendResult(uint8_t result, uint16_t ping)
{
    UiEvent event;
    event.type = UiEvent::Type::SendResult;
    event.success = result != 0;
    event.ping = ping;
    snprintf(event.text, sizeof(event.text),
             result ? "Delivered in %u ms" : "Delivery failed", ping);
    postUiEvent(event);
}

void IRAM_ATTR onPmuInterrupt()
{
    portENTER_CRITICAL_ISR(&irqMux);
    pmuIrq = true;
    portEXIT_CRITICAL_ISR(&irqMux);
}

void IRAM_ATTR onMotionInterrupt()
{
    portENTER_CRITICAL_ISR(&irqMux);
    motionIrq = true;
    portEXIT_CRITICAL_ISR(&irqMux);
}

bool takeIrqFlag(volatile bool &flag)
{
    portENTER_CRITICAL(&irqMux);
    const bool value = flag;
    flag = false;
    portEXIT_CRITICAL(&irqMux);
    return value;
}

void wakeDisplay()
{
    wakeRequested.store(false, std::memory_order_release);
    if (!displayOn)
    {
        watch.setBrightness(settings.brightness);
        displayOn = true;
    }
    lv_disp_trig_activity(nullptr);
}

void turnDisplayOff()
{
    if (!displayOn)
        return;
    watch.setBrightness(0);
    displayOn = false;
}

void vibrateNotification()
{
    if (!settings.haptic)
        return;
    watch.setWaveform(0, 15);
    watch.setWaveform(1, 0);
    watch.run();
}

lv_obj_t *makePage(uint8_t column, uint8_t total)
{
    lv_dir_t direction = LV_DIR_NONE;
    if (column > 0)
        direction = static_cast<lv_dir_t>(direction | LV_DIR_LEFT);
    if (column + 1 < total)
        direction = static_cast<lv_dir_t>(direction | LV_DIR_RIGHT);
    lv_obj_t *page = lv_tileview_add_tile(tileview, column, 0, direction);
    lv_obj_add_style(page, &pageStyle, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

lv_obj_t *makeTitle(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &titleStyle, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 3);
    return label;
}

lv_obj_t *makeCard(lv_obj_t *parent, int x, int y, int width, int height)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, width, height);
    lv_obj_add_style(card, &cardStyle, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void updateSendButtonState()
{
    if (!sendButton)
        return;
    if (routes.empty() || selectedTarget == UINT16_MAX || sending)
        lv_obj_add_state(sendButton, LV_STATE_DISABLED);
    else
        lv_obj_clear_state(sendButton, LV_STATE_DISABLED);
}

void updateRouteWidgets()
{
    size_t direct = 0;
    for (const NeighborRecord &route : routes)
        if (route.distance == 1)
            ++direct;

    if (homeMetrics)
        lv_label_set_text_fmt(
            homeMetrics, "%u routes  •  %u direct  •  node %u",
            static_cast<unsigned>(routes.size()),
            static_cast<unsigned>(direct), NODE_ID);
    if (meshSummary)
        lv_label_set_text_fmt(
            meshSummary, "%u reachable  •  %u direct",
            static_cast<unsigned>(routes.size()),
            static_cast<unsigned>(direct));

    if (targetDropdown)
    {
        uint16_t previous = selectedTarget;
        std::string options;
        for (const NeighborRecord &route : routes)
        {
            char option[48];
            snprintf(option, sizeof(option), "Node %u  ·  %u hop%s",
                     route.id, route.distance, route.distance == 1 ? "" : "s");
            if (!options.empty())
                options.push_back('\n');
            options += option;
        }
        if (options.empty())
            options = "No reachable nodes";
        lv_dropdown_set_options(targetDropdown, options.c_str());

        selectedTarget = UINT16_MAX;
        for (size_t index = 0; index < routes.size(); ++index)
        {
            if (routes[index].id == previous)
            {
                selectedTarget = previous;
                lv_dropdown_set_selected(targetDropdown, index);
                break;
            }
        }
        if (selectedTarget == UINT16_MAX && !routes.empty())
        {
            selectedTarget = routes.front().id;
            lv_dropdown_set_selected(targetDropdown, 0);
        }
    }

    if (meshTable)
    {
        const size_t visible = std::min(routes.size(), MAX_TABLE_ROUTES);
        lv_table_set_row_cnt(meshTable, visible + 1);
        lv_table_set_cell_value(meshTable, 0, 0, "Node");
        lv_table_set_cell_value(meshTable, 0, 1, "Via");
        lv_table_set_cell_value(meshTable, 0, 2, "Hop");
        for (size_t index = 0; index < visible; ++index)
        {
            char id[12], via[12], hops[8];
            snprintf(id, sizeof(id), "%u", routes[index].id);
            snprintf(via, sizeof(via), "%u", routes[index].from);
            snprintf(hops, sizeof(hops), "%u", routes[index].distance);
            lv_table_set_cell_value(meshTable, index + 1, 0, id);
            lv_table_set_cell_value(meshTable, index + 1, 1, via);
            lv_table_set_cell_value(meshTable, index + 1, 2, hops);
        }
    }
    updateSendButtonState();
}

void refreshRoutes(bool force)
{
    DTPK *dtpk = DTPK::getInstance();
    if (!dtpk)
        return;
    std::vector<NeighborRecord> current = dtpk->getNeighbours();
    std::sort(current.begin(), current.end(), routeLess);
    if (!force && routesEqual(routes, current))
        return;
    routes = std::move(current);
    updateRouteWidgets();
    Bluetooth::getInstance()->sendNeighborsUpdate();
}

void processUiEvents()
{
    if (!uiQueue)
        return;
    UiEvent event;
    while (xQueueReceive(uiQueue, &event, 0) == pdTRUE)
    {
        wakeDisplay();
        if (event.type == UiEvent::Type::Incoming)
        {
            if (homeEvent)
                lv_label_set_text_fmt(homeEvent, "From %u: %s",
                                      event.sender, event.text);
            if (inboxLabel)
                lv_label_set_text_fmt(inboxLabel, "Latest from %u\n%s",
                                      event.sender, event.text);
            vibrateNotification();
        }
        else if (event.type == UiEvent::Type::SendResult)
        {
            sending = false;
            if (sendStatus)
                lv_label_set_text(sendStatus, event.text);
            if (homeEvent)
                lv_label_set_text(homeEvent, event.text);
            if (event.success)
                vibrateNotification();
            updateSendButtonState();
        }
        else
        {
            if (homeEvent)
                lv_label_set_text(homeEvent, event.text);
        }
    }
}

void processPmu()
{
    if (!takeIrqFlag(pmuIrq))
        return;
    watch.readPMU();
    if (watch.isVbusInsertIrq())
    {
        usbConnected = true;
        postSystemMessage("USB power connected");
    }
    if (watch.isVbusRemoveIrq())
    {
        usbConnected = false;
        postSystemMessage("Running on battery");
    }
    if (watch.isPekeyShortPressIrq())
    {
        if (displayOn)
            turnDisplayOff();
        else
            wakeDisplay();
    }
    if (watch.isPekeyLongPressIrq())
    {
        if (settingsDirty)
            saveSettings();
        watch.clearPMU();
        watch.shutdown();
    }
    watch.clearPMU();
}

void processMotion()
{
    if (!takeIrqFlag(motionIrq))
        return;
    (void)watch.readBMA();
    stepCount = watch.getPedometerCounter();
    if (watch.isDoubleTap())
        wakeDisplay();
}

void updateHomeAndDiagnostics()
{
    struct tm timeInfo{};
    watch.getDateTime(&timeInfo);
    char timeText[12];
    char dateText[28];
    strftime(timeText, sizeof(timeText), "%H:%M", &timeInfo);
    strftime(dateText, sizeof(dateText), "%a, %d %b", &timeInfo);
    lv_label_set_text(homeTime, timeText);
    lv_label_set_text(homeDate, dateText);

    int battery = watch.getBatteryPercent();
    if (battery < 0)
        battery = 0;
    const bool ble = Bluetooth::getInstance()->deviceIsConnected();
    lv_label_set_text_fmt(homeStatus, "%s  %d%%  •  BLE %s",
                          usbConnected ? "USB" : "BAT", battery,
                          ble ? "connected" : "ready");

    stepCount = watch.getPedometerCounter();
    const int temperature = static_cast<int>(watch.readAccelTemp());
    lv_label_set_text_fmt(homeMetrics,
                          "%u routes  •  %u steps  •  %d°C sensor",
                          static_cast<unsigned>(routes.size()),
                          static_cast<unsigned>(stepCount), temperature);

    MAC *mac = MAC::getInstance();
    Bluetooth *bleGateway = Bluetooth::getInstance();
    if (mac && diagnosticLabel)
    {
        const MAC::Diagnostics &radioDiagnostics = mac->getDiagnostics();
        const BluetoothDiagnostics bluetoothDiagnostics =
            bleGateway->getDiagnostics();
        lv_label_set_text_fmt(
            diagnosticLabel,
            "Radio %s E%ld C%lu RX%lu/%lu\n"
            "BLE %s MTU%u D%lu/%lu/%lu\n"
            "Node%u C%u SF%u BW%.0f",
            mac->isReady() ? "ready" : "fault",
            static_cast<long>(mac->getLastRadioError()),
            static_cast<unsigned long>(radioDiagnostics.radioCommandErrors),
            static_cast<unsigned long>(radioDiagnostics.rxReadErrors),
            static_cast<unsigned long>(radioDiagnostics.rxTooShort),
            bleGateway->deviceIsConnected() ? "connected" : "advertising",
            bleGateway->negotiatedMtu(),
            static_cast<unsigned long>(bluetoothDiagnostics.droppedWrites),
            static_cast<unsigned long>(bluetoothDiagnostics.droppedInboundMessages),
            static_cast<unsigned long>(bluetoothDiagnostics.droppedControlNotifications),
            NODE_ID, RADIO_CHANNEL, RADIO_SF, RADIO_BANDWIDTH_KHZ);
    }
}

void onTargetChanged(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED || routes.empty())
        return;
    const uint16_t index = lv_dropdown_get_selected(lv_event_get_target(event));
    if (index < routes.size())
        selectedTarget = routes[index].id;
    updateSendButtonState();
}

void onQuickMessageChanged(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED)
        selectedQuickMessage = std::min<uint8_t>(
            QUICK_MESSAGES.size() - 1,
            lv_dropdown_get_selected(lv_event_get_target(event)));
}

void onSendClicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || sending ||
        selectedTarget == UINT16_MAX || !DTPK::getInstance())
        return;

    const char *payload = QUICK_MESSAGES[selectedQuickMessage];
    sending = true;
    updateSendButtonState();
    lv_label_set_text_fmt(sendStatus, "Sending to node %u…", selectedTarget);
    const uint16_t id = DTPK::getInstance()->sendPacket(
        selectedTarget,
        reinterpret_cast<unsigned char *>(const_cast<char *>(payload)),
        strlen(payload), 60000, true, onSendResult);
    if (id == 0)
    {
        sending = false;
        lv_label_set_text(sendStatus, "Protocol is busy");
        updateSendButtonState();
    }
}

void onRefreshClicked(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED)
    {
        refreshRoutes(true);
        lv_label_set_text(scanStatus, "Route table refreshed");
    }
}

void onScanClicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
        return;
    MAC *mac = MAC::getInstance();
    if (!mac)
        return;

    // MAC calibrates every configured channel during initialization. Show that
    // snapshot without retuning the live radio and dropping mesh traffic.
    const uint8_t channels = mac->getNumberOfChannels();
    if (channels == 0)
    {
        lv_label_set_text(scanStatus, "No channels");
        return;
    }
    int minimum = INT_MAX;
    int maximum = INT_MIN;
    for (uint8_t channel = 0; channel < channels; ++channel)
    {
        const int noise = mac->getNoiseFloorOfChannel(channel);
        minimum = std::min(minimum, noise);
        maximum = std::max(maximum, noise);
    }
    lv_label_set_text_fmt(
        scanStatus, "Noise\n%d..%d dBm", minimum, maximum);
}

void onBrightnessChanged(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
        return;
    settings.brightness = static_cast<uint8_t>(
        lv_slider_get_value(lv_event_get_target(event)));
    watch.setBrightness(settings.brightness);
    lv_label_set_text_fmt(brightnessValue, "%u%%", settings.brightness);
    markSettingsDirty();
}

void onTimeoutChanged(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
        return;
    settings.timeoutIndex = std::min<uint8_t>(
        SCREEN_TIMEOUTS.size() - 1,
        lv_dropdown_get_selected(lv_event_get_target(event)));
    markSettingsDirty();
}

void onHapticChanged(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED)
        return;
    settings.haptic = lv_obj_has_state(
        lv_event_get_target(event), LV_STATE_CHECKED);
    markSettingsDirty();
    if (settings.haptic)
        vibrateNotification();
}

void onScreenOffClicked(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        turnDisplayOff();
}

void initializeStyles()
{
    lv_style_init(&pageStyle);
    lv_style_set_bg_color(&pageStyle, lv_color_hex(0x07111f));
    lv_style_set_bg_opa(&pageStyle, LV_OPA_COVER);
    lv_style_set_text_color(&pageStyle, lv_color_hex(0xe8f0ff));
    lv_style_set_border_width(&pageStyle, 0);
    lv_style_set_pad_all(&pageStyle, 8);

    lv_style_init(&cardStyle);
    lv_style_set_bg_color(&cardStyle, lv_color_hex(0x10233a));
    lv_style_set_bg_opa(&cardStyle, LV_OPA_COVER);
    lv_style_set_radius(&cardStyle, 12);
    lv_style_set_border_width(&cardStyle, 1);
    lv_style_set_border_color(&cardStyle, lv_color_hex(0x24496d));
    lv_style_set_pad_all(&cardStyle, 8);
    lv_style_set_text_color(&cardStyle, lv_color_hex(0xe8f0ff));

    lv_style_init(&titleStyle);
    lv_style_set_text_font(&titleStyle, &lv_font_montserrat_18);
    lv_style_set_text_color(&titleStyle, lv_color_hex(0x7cc8ff));

    lv_style_init(&primaryButtonStyle);
    lv_style_set_bg_color(&primaryButtonStyle, lv_color_hex(0x1769aa));
    lv_style_set_bg_opa(&primaryButtonStyle, LV_OPA_COVER);
    lv_style_set_radius(&primaryButtonStyle, 10);
    lv_style_set_text_color(&primaryButtonStyle, lv_color_white());
}

void createHomePage(lv_obj_t *page)
{
    makeTitle(page, "PICOPOD  •  1/4");
    homeTime = lv_label_create(page);
    lv_obj_set_style_text_font(homeTime, &lv_font_montserrat_36, LV_PART_MAIN);
    lv_obj_align(homeTime, LV_ALIGN_TOP_MID, 0, 29);

    homeDate = lv_label_create(page);
    lv_obj_set_style_text_color(homeDate, lv_color_hex(0x9db4cc), LV_PART_MAIN);
    lv_obj_align(homeDate, LV_ALIGN_TOP_MID, 0, 72);

    lv_obj_t *statusCard = makeCard(page, 8, 98, 224, 52);
    homeStatus = lv_label_create(statusCard);
    lv_obj_set_width(homeStatus, 206);
    lv_label_set_long_mode(homeStatus, LV_LABEL_LONG_CLIP);
    lv_obj_align(homeStatus, LV_ALIGN_TOP_LEFT, 0, 0);
    homeMetrics = lv_label_create(statusCard);
    lv_obj_set_width(homeMetrics, 206);
    lv_label_set_long_mode(homeMetrics, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(homeMetrics, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(homeMetrics, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *eventCard = makeCard(page, 8, 157, 224, 72);
    homeEvent = lv_label_create(eventCard);
    lv_obj_set_width(homeEvent, 206);
    lv_label_set_long_mode(homeEvent, LV_LABEL_LONG_WRAP);
    lv_label_set_text(homeEvent, "Mesh ready. Swipe for messages and routes.");
}

void createMessagesPage(lv_obj_t *page)
{
    makeTitle(page, "MESSAGES  •  2/4");

    targetDropdown = lv_dropdown_create(page);
    lv_obj_set_pos(targetDropdown, 8, 29);
    lv_obj_set_size(targetDropdown, 224, 38);
    lv_dropdown_set_options(targetDropdown, "No reachable nodes");
    lv_obj_add_event_cb(targetDropdown, onTargetChanged,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    quickMessageDropdown = lv_dropdown_create(page);
    lv_obj_set_pos(quickMessageDropdown, 8, 73);
    lv_obj_set_size(quickMessageDropdown, 224, 38);
    lv_dropdown_set_options(
        quickMessageDropdown,
        "Hello\nOK\nNeed help\nWhere are you?\nI am safe");
    lv_obj_add_event_cb(quickMessageDropdown, onQuickMessageChanged,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    sendButton = lv_btn_create(page);
    lv_obj_set_pos(sendButton, 8, 118);
    lv_obj_set_size(sendButton, 88, 42);
    lv_obj_add_style(sendButton, &primaryButtonStyle, LV_PART_MAIN);
    lv_obj_add_event_cb(sendButton, onSendClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *sendLabel = lv_label_create(sendButton);
    lv_label_set_text(sendLabel, "SEND");
    lv_obj_center(sendLabel);

    sendStatus = lv_label_create(page);
    lv_obj_set_pos(sendStatus, 106, 121);
    lv_obj_set_size(sendStatus, 126, 38);
    lv_label_set_long_mode(sendStatus, LV_LABEL_LONG_WRAP);
    lv_label_set_text(sendStatus, "Select a node");

    lv_obj_t *inboxCard = makeCard(page, 8, 166, 224, 63);
    inboxLabel = lv_label_create(inboxCard);
    lv_obj_set_width(inboxLabel, 206);
    lv_label_set_long_mode(inboxLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(inboxLabel,
                      "No messages yet. Use the phone BLE client for arbitrary text.");
}

void createMeshPage(lv_obj_t *page)
{
    makeTitle(page, "MESH  •  3/4");
    meshSummary = lv_label_create(page);
    lv_obj_align(meshSummary, LV_ALIGN_TOP_LEFT, 10, 29);
    lv_label_set_text(meshSummary, "0 reachable");

    meshTable = lv_table_create(page);
    lv_obj_set_pos(meshTable, 8, 51);
    lv_obj_set_size(meshTable, 224, 126);
    lv_table_set_col_cnt(meshTable, 3);
    lv_table_set_row_cnt(meshTable, 1);
    lv_table_set_col_width(meshTable, 0, 75);
    lv_table_set_col_width(meshTable, 1, 75);
    lv_table_set_col_width(meshTable, 2, 55);

    lv_obj_t *refresh = lv_btn_create(page);
    lv_obj_set_pos(refresh, 8, 184);
    lv_obj_set_size(refresh, 82, 38);
    lv_obj_add_event_cb(refresh, onRefreshClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *refreshLabel = lv_label_create(refresh);
    lv_label_set_text(refreshLabel, "REFRESH");
    lv_obj_center(refreshLabel);

    scanButton = lv_btn_create(page);
    lv_obj_set_pos(scanButton, 96, 184);
    lv_obj_set_size(scanButton, 64, 38);
    lv_obj_add_event_cb(scanButton, onScanClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *scanLabel = lv_label_create(scanButton);
    lv_label_set_text(scanLabel, "NOISE");
    lv_obj_center(scanLabel);

    scanStatus = lv_label_create(page);
    lv_obj_set_pos(scanStatus, 166, 181);
    lv_obj_set_size(scanStatus, 68, 46);
    lv_obj_set_style_text_font(scanStatus, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_label_set_long_mode(scanStatus, LV_LABEL_LONG_WRAP);
    lv_label_set_text(scanStatus, "Noise\nsnapshot");
}

void createSettingsPage(lv_obj_t *page)
{
    makeTitle(page, "SETTINGS  •  4/4");

    lv_obj_t *brightnessLabel = lv_label_create(page);
    lv_label_set_text(brightnessLabel, "Brightness");
    lv_obj_set_pos(brightnessLabel, 10, 31);
    brightnessValue = lv_label_create(page);
    lv_obj_set_pos(brightnessValue, 188, 31);
    lv_label_set_text_fmt(brightnessValue, "%u%%", settings.brightness);

    brightnessSlider = lv_slider_create(page);
    lv_obj_set_pos(brightnessSlider, 10, 53);
    lv_obj_set_size(brightnessSlider, 220, 12);
    lv_slider_set_range(brightnessSlider, SETTINGS_BRIGHTNESS_MIN, 100);
    lv_slider_set_value(brightnessSlider, settings.brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightnessSlider, onBrightnessChanged,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *timeoutLabel = lv_label_create(page);
    lv_label_set_text(timeoutLabel, "Screen timeout");
    lv_obj_set_pos(timeoutLabel, 10, 76);
    timeoutDropdown = lv_dropdown_create(page);
    lv_obj_set_pos(timeoutDropdown, 116, 70);
    lv_obj_set_size(timeoutDropdown, 114, 38);
    lv_dropdown_set_options(timeoutDropdown,
                            "30 sec\n1 min\n2 min\n5 min\nAlways on");
    lv_dropdown_set_selected(timeoutDropdown, settings.timeoutIndex);
    lv_obj_add_event_cb(timeoutDropdown, onTimeoutChanged,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *hapticLabel = lv_label_create(page);
    lv_label_set_text(hapticLabel, "Message vibration");
    lv_obj_set_pos(hapticLabel, 10, 121);
    hapticSwitch = lv_switch_create(page);
    lv_obj_set_pos(hapticSwitch, 178, 114);
    if (settings.haptic)
        lv_obj_add_state(hapticSwitch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(hapticSwitch, onHapticChanged,
                        LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *offButton = lv_btn_create(page);
    lv_obj_set_pos(offButton, 10, 151);
    lv_obj_set_size(offButton, 94, 35);
    lv_obj_add_event_cb(offButton, onScreenOffClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *offLabel = lv_label_create(offButton);
    lv_label_set_text(offLabel, "SCREEN OFF");
    lv_obj_center(offLabel);

    diagnosticLabel = lv_label_create(page);
    lv_obj_set_pos(diagnosticLabel, 10, 190);
    lv_obj_set_size(diagnosticLabel, 224, 46);
    lv_obj_set_style_text_font(diagnosticLabel, &lv_font_montserrat_12,
                               LV_PART_MAIN);
    lv_label_set_long_mode(diagnosticLabel, LV_LABEL_LONG_CLIP);
    lv_label_set_text(diagnosticLabel, "Diagnostics starting…");
}

void createUi()
{
    initializeStyles();
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, 240, 240);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x07111f), LV_PART_MAIN);
    lv_obj_set_style_border_width(tileview, 0, LV_PART_MAIN);

    constexpr uint8_t pageCount = 4;
    createHomePage(makePage(0, pageCount));
    createMessagesPage(makePage(1, pageCount));
    createMeshPage(makePage(2, pageCount));
    createSettingsPage(makePage(3, pageCount));
    lv_obj_set_tile(tileview, lv_obj_get_child(tileview, 0), LV_ANIM_OFF);
}

void configureSensors()
{
    watch.configAccelerometer();
    watch.enableAccelerometer();
    watch.enablePedometer();
    watch.configInterrupt();
    watch.enableFeature(
        SensorBMA423::FEATURE_STEP_CNTR |
            SensorBMA423::FEATURE_ANY_MOTION |
            SensorBMA423::FEATURE_NO_MOTION |
            SensorBMA423::FEATURE_ACTIVITY |
            SensorBMA423::FEATURE_TILT |
            SensorBMA423::FEATURE_WAKEUP,
        true);
    watch.enablePedometerIRQ();
    watch.enableTiltIRQ();
    watch.enableWakeupIRQ();
    watch.enableAnyNoMotionIRQ();
    watch.enableActivityIRQ();
    watch.attachBMA(onMotionInterrupt);
}

void configurePmu()
{
    watch.clearPMU();
    watch.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    watch.enableIRQ(
        XPOWERS_AXP2101_VBUS_INSERT_IRQ |
        XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
        XPOWERS_AXP2101_PKEY_LONG_IRQ |
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ |
        XPOWERS_AXP2101_BAT_CHG_START_IRQ);
    watch.attachPMU(onPmuInterrupt);
}
} // namespace

bool watchSetup()
{
    if (!watch.begin(&Serial))
    {
        Serial.println("T-Watch hardware initialization failed");
        return false;
    }
    setCpuFrequencyMhz(160);
    loadSettings();
    watch.setBrightness(settings.brightness);
    usbConnected = watch.isVbusIn();

    configurePmu();
    configureSensors();

    if (!MAC::initialize(
            radio, NODE_ID, MACRegion::EU868, RADIO_CHANNEL, RADIO_SF,
            RADIO_BANDWIDTH_KHZ, RADIO_SQUELCH_DB, RADIO_POWER_DBM,
            RADIO_CODING_RATE))
    {
        Serial.printf("MAC initialization failed: %d\n",
                      MAC::getInstance()
                          ? MAC::getInstance()->getLastRadioError()
                          : INT16_MIN);
        return false;
    }
    if (!DTPK::initialize(20, nextBootSequence(), true))
    {
        Serial.println("DTProtocol initialization failed");
        return false;
    }

    if (!beginLvglHelper(false))
        return false;
    uiQueue = xQueueCreate(8, sizeof(UiEvent));
    if (!uiQueue)
    {
        Serial.println("UI event queue allocation failed");
        return false;
    }
    createUi();

    Bluetooth::initialize();
    Bluetooth *bluetooth = Bluetooth::getInstance();
    bluetooth->setDeviceName("DTPK-LoraWatch");
    bluetooth->setDTPKPacketCallback(onDTPKPacket);
    if (!bluetooth->setup())
        postSystemMessage("BLE unavailable; LoRa remains active");

    refreshRoutes(true);
    updateHomeAndDiagnostics();
    lv_disp_trig_activity(nullptr);
    Serial.printf("Picopod watch node %u ready: EU868 C%u SF%u BW%.0f\n",
                  NODE_ID, RADIO_CHANNEL, RADIO_SF, RADIO_BANDWIDTH_KHZ);
    return true;
}

void watchLoop()
{
    Bluetooth::getInstance()->loop();
    processPmu();
    processMotion();
    processUiEvents();

    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastRouteRefresh) >= ROUTE_REFRESH_MS)
    {
        lastRouteRefresh = now;
        refreshRoutes(false);
    }
    if (static_cast<uint32_t>(now - lastSensorRefresh) >= SENSOR_REFRESH_MS)
    {
        lastSensorRefresh = now;
        updateHomeAndDiagnostics();
    }
    if (static_cast<uint32_t>(now - lastDiagnosticRefresh) >=
        DIAGNOSTIC_REFRESH_MS)
    {
        lastDiagnosticRefresh = now;
        updateHomeAndDiagnostics();
    }

    if (wakeRequested.exchange(false, std::memory_order_acq_rel))
        wakeDisplay();

    saveSettingsWhenDue(now);

    const uint32_t timeout = SCREEN_TIMEOUTS[settings.timeoutIndex];
    if (displayOn && timeout != UINT32_MAX &&
        lv_disp_get_inactive_time(nullptr) >= timeout && !sending)
        turnDisplayOff();

    if (!displayOn && watch.getTouched())
        wakeDisplay();

    if (displayOn ||
        static_cast<uint32_t>(now - lastDisplayService) >= DISPLAY_OFF_SERVICE_MS)
    {
        lastDisplayService = now;
        (void)lv_timer_handler();
    }
    delay(displayOn ? 2 : 10);
}
