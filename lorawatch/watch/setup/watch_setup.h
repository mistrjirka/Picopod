#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <sntp.h>
#include "SensorCommon.tpp"
#include <lcmm.h>
#include <mac.h>
#include <DTPK.h>
#ifdef ENABLE_IR_SENDER
#include <IRsend.h>
IRsend irsend(BOARD_IR_PIN);
#endif
#include <driver/i2s.h>
#include <esp_vad.h>
#include <time.h>
#ifdef ENABLE_PLAYER
#include <AudioFileSourcePROGMEM.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSPIFFS.h>
#endif

#ifndef WATCH_SETUP_H
#define WATCH_SETUP_H
#define ENABLE_PLAYER
#define ENABLE_IR_SENDER

#define LVGL_MESSAGE_PROGRESS_CHANGED_ID        (88)
#define DEFAULT_RECORD_FILENAME                 "/rec.wav"
#define AUDIO_DATA                              boot_music
#define RADIO_TRANSMIT_PAGE_ID                  1

#define DEFAULT_SCREEN_TIMEOUT                  135*1000
#define DEFAULT_COLOR                           (lv_color_make(252, 218, 72))
#define VAD_FRAME_LENGTH_MS                     30
#define VAD_BUFFER_LENGTH                       (VAD_FRAME_LENGTH_MS * MIC_I2S_SAMPLE_RATE / 1000)

void createChargeUI();
void updateTableDTP();
void updateDropdown();
void radioSendMessage(lv_obj_t *parent);

void watchSetup();
void SensorHandler();
static void radioSendAndReceivePage(lv_obj_t *parent);

typedef bool (*player_cb_t)(void);
static player_cb_t player_task_cb = NULL;
static bool playWAV();
static bool playMP3();

void factory_ui();

void productPinmap(lv_obj_t *parent);
void analogclock(lv_obj_t *parent);
void analogclock2(lv_obj_t *parent);
void analogclock3(lv_obj_t *parent);


void digitalClock(lv_obj_t *parent);
void digitalClock2(lv_obj_t *parent);

void devicesInformation(lv_obj_t *parent);
void wifiscan(lv_obj_t *parent);
void musicPlay(lv_obj_t *parent);
void radioPingPong(lv_obj_t *parent);
void irRemoteVeiw(lv_obj_t *parent);
void datetimeVeiw(lv_obj_t *parent);
void lilygo_qrcode(lv_obj_t *parent);

void settingPMU();
void settingSensor();
void settingRadio();
void settingPlayer();
void settingIRRemote();
void settingButtonStyle();
void PMUHandler();
void lowPowerEnergyHandler();
void destoryChargeUI();


static lv_obj_t *battery_percent;
static lv_obj_t *weather_celsius;
static lv_obj_t *step_counter;

static lv_obj_t *hour_img;
static lv_obj_t *min_img;
static lv_obj_t *sec_img;
static lv_obj_t *tileview;
static lv_obj_t *radio_ta;
static lv_obj_t *wifi_table_list;
static lv_obj_t *label_datetime;
static lv_obj_t *charge_cont;

static lv_timer_t *transmitTask;
static lv_timer_t *clockTimer;
static TaskHandle_t playerTaskHandler;
static TaskHandle_t vadTaskHandler;

static lv_style_t button_default_style;
static lv_style_t button_press_style;
// Save the ID of the current page
static uint8_t pageId = 0;
// Flag used to indicate whether to use light sleep, currently unavailable
static bool lightSleep = false;
// Flag used for acceleration interrupt status
static bool sportsIrq = false;
// Flag used to indicate whether recording is enabled
static bool recordFlag = false;
// Flag used for PMU interrupt trigger status
static bool pmuIrq = false;
// Flag used to select whether to turn off the screen
static bool canScreenOff = true;
// Flag used to detect USB insertion status
static bool usbPlugIn = false;
// Flag to indicate that a packet was sent or received
static bool radioTransmitFlag = false;
// Save transmission states between loops
static int transmissionState = RADIOLIB_ERR_NONE;
// Flag to indicate transmission or reception state
static bool transmitFlag = false;
// Save pedometer steps
static uint32_t stepCounter = 0;
// Save Radio Transmit Interval
static uint32_t configTransmitInterval = 0;
// Save brightness value
static RTC_DATA_ATTR int brightnessLevel = 0;
// Vad detecte values
static int16_t *vad_buff = NULL;
static vad_handle_t vad_inst = NULL;
static bool vad_detect = false;
const size_t vad_buffer_size = VAD_BUFFER_LENGTH * sizeof(short);
static lv_obj_t *vad_btn_label;
static lv_obj_t *vad_btn;
static uint32_t vad_detected_counter = 0;

#endif