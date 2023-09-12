#include "watch_setup.h"
#define ID 3
const char *ssid = "highground";
const char *password = "hellothere";

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";

const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

bool time_ready = false;
bool wifi_turned_on = true;
LV_IMG_DECLARE(clock_face);
LV_IMG_DECLARE(clock_hour_hand);
LV_IMG_DECLARE(clock_minute_hand);
LV_IMG_DECLARE(clock_second_hand);

LV_IMG_DECLARE(watch_if);
LV_IMG_DECLARE(watch_bg);
LV_IMG_DECLARE(watch_if_hour);
LV_IMG_DECLARE(watch_if_min);
LV_IMG_DECLARE(watch_if_sec);

LV_IMG_DECLARE(watch_if_bg2);
LV_IMG_DECLARE(watch_if_hour2);
LV_IMG_DECLARE(watch_if_min2);
LV_IMG_DECLARE(watch_if_sec2);

LV_FONT_DECLARE(font_siegra);
LV_FONT_DECLARE(font_sandbox);
LV_FONT_DECLARE(font_jetBrainsMono);
LV_FONT_DECLARE(font_firacode_60);
LV_FONT_DECLARE(font_ununtu_18);

LV_IMG_DECLARE(img_usb_plug);

LV_IMG_DECLARE(charge_done_battery);

LV_IMG_DECLARE(watch_if_5);
LV_IMG_DECLARE(watch_if_6);

LV_IMG_DECLARE(watch_if_8);
void timeavailable(struct timeval *t)
{
    watch.hwClockWrite();
    time_ready = true;
}
void synchronize()
{
    // const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

    sntp_set_time_sync_notification_cb(timeavailable);

    sntp_servermode_dhcp(1); // (optional)

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    // configTzTime(time_zone, ntpServer1, ntpServer2);

    WiFi.begin(ssid, password);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 8)
    {
        delay(500);
        // //Serial.print(".");
    }
    if (i >= 7)
    {
        // //Serial.println("timeout turning wifi off");
        WiFi.mode(WIFI_MODE_NULL);
        wifi_turned_on = false;
    }
}

void watchSetup()
{
    // Serial.begin(115200);

    // Stop wifi
    watch.begin();
    WiFi.mode(WIFI_MODE_NULL);

    btStop();

    setCpuFrequencyMhz(160);

    settingPMU();

    settingSensor();

    SX1262 module = watch.getMod();
    // MAC::initialize(module, 1, 2);
    MAC::initialize(
        module,
        ID,
        2,
        9,
        125.0,
        15,
        22,
        7);

    // //Serial.println("setup MAC");

    beginLvglHelper(false);

    settingButtonStyle();

    factory_ui();

    // synchronize();

    usbPlugIn = watch.isVbusIn();
}

void SensorHandler()
{
    DTP::getInstance()->loop();
    updateDropdown();

    if (sportsIrq)
    {
        sportsIrq = false;
        // The interrupt status must be read after an interrupt is detected
        uint16_t status = watch.readBMA();
        // //Serial.printf("Accelerometer interrupt mask : 0x%x\n", status);

        if (watch.isPedometer())
        {
            stepCounter = watch.getPedometerCounter();
            // //Serial.printf("Step count interrupt,step Counter:%u\n", stepCounter);
        }
        if (watch.isActivity())
        {
            // //Serial.println("Activity interrupt");
        }
        if (watch.isTilt())
        {
            // //Serial.println("Tilt interrupt");
        }
        if (watch.isDoubleTap())
        {
            // //Serial.println("DoubleTap interrupt");
        }
        if (watch.isAnyNoMotion())
        {
            // //Serial.println("Any motion / no motion interrupt");
        }
    }
    updateTableDTP();
}

static void charge_anim_cb(void *obj, int32_t v)
{
    lv_obj_t *arc = (lv_obj_t *)obj;
    static uint32_t last_check_inteval;
    static int battery_percent;
    if (last_check_inteval < millis())
    {
        battery_percent = watch.getBatteryPercent();
        lv_obj_t *label_percent = (lv_obj_t *)lv_obj_get_user_data(arc);
        lv_label_set_text_fmt(label_percent, "%d%%", battery_percent);
        if (battery_percent == 100)
        {
            lv_obj_t *img = (lv_obj_t *)lv_obj_get_user_data(label_percent);
            lv_anim_del(arc, charge_anim_cb);
            lv_arc_set_value(arc, 100);
            lv_img_set_src(img, &charge_done_battery);
        }
        last_check_inteval = millis() + 2000;
    }
    if (v >= battery_percent)
    {
        return;
    }
    lv_arc_set_value(arc, v);
}

void tileview_change_cb(lv_event_t *e)
{
    static uint16_t lastPageID = 0;
    lv_obj_t *tileview = lv_event_get_target(e);
    pageId = lv_obj_get_index(lv_tileview_get_tile_act(tileview));
    lv_event_code_t c = lv_event_get_code(e);
    uint32_t count = lv_obj_get_child_cnt(tileview);
    // //Serial.print(" Count:");
    // //Serial.print(count);
    // //Serial.print(" pageId:");
    // //Serial.println(pageId);

    switch (pageId)
    {
    case RADIO_TRANSMIT_PAGE_ID:
        canScreenOff = false;
        break;
    default:
        canScreenOff = true;
        break;
    }
    lastPageID = pageId;
    // //Serial.print(" pageId:");
}

void lowPowerEnergyHandler()
{
    // //Serial.println("Enter light sleep mode!");
    brightnessLevel = watch.getBrightness();
    watch.decrementBrightness(0);

    // //Serial.println("DEcremented brigtness!");

    watch.clearPMU();
    // //Serial.println("Cleared pmu!");

    watch.configreFeatureInterrupt(
        SensorBMA423::INT_STEP_CNTR |    // Pedometer interrupt
            SensorBMA423::INT_ACTIVITY | // Activity interruption
            SensorBMA423::INT_TILT |     // Tilt interrupt
            // SensorBMA423::INT_WAKEUP |      // DoubleTap interrupt
            SensorBMA423::INT_ANY_NO_MOTION, // Any  motion / no motion interrupt
        false);

    sportsIrq = false;
    pmuIrq = false;
    // TODO: Low power consumption not debugged
    if (lightSleep)
    {
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        // esp_sleep_enable_ext1_wakeup(1ULL << BOARD_BMA423_INT1, ESP_EXT1_WAKEUP_ANY_HIGH);
        // esp_sleep_enable_ext1_wakeup(1ULL << BOARD_PMU_INT, ESP_EXT1_WAKEUP_ALL_LOW);

        gpio_wakeup_enable((gpio_num_t)BOARD_PMU_INT, GPIO_INTR_LOW_LEVEL);
        gpio_wakeup_enable((gpio_num_t)BOARD_BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
    }
    else
    {

        setCpuFrequencyMhz(10);
        // setCpuFrequencyMhz(80);
        while (!pmuIrq && !sportsIrq && !watch.getTouched())
        {
            delay(300);
            // gpio_wakeup_enable ((gpio_num_t)BOARD_TOUCH_INT, GPIO_INTR_LOW_LEVEL);
            // esp_sleep_enable_timer_wakeup(3 * 1000);
            // esp_light_sleep_start();
        }

        setCpuFrequencyMhz(160);
    }

    // Clear Interrupts in Loop
    // watch.readBMA();
    // watch.clearPMU();

    watch.configreFeatureInterrupt(
        SensorBMA423::INT_STEP_CNTR |    // Pedometer interrupt
            SensorBMA423::INT_ACTIVITY | // Activity interruption
            SensorBMA423::INT_TILT |     // Tilt interrupt
            // SensorBMA423::INT_WAKEUP |      // DoubleTap interrupt
            SensorBMA423::INT_ANY_NO_MOTION, // Any  motion / no motion interrupt
        true);

    lv_disp_trig_activity(NULL);
    // Run once
    lv_task_handler();

    watch.incrementalBrightness(brightnessLevel);
}

void createChargeUI()
{
    if (charge_cont)
    {
        return;
    }

    static lv_style_t cont_style;
    lv_style_init(&cont_style);
    lv_style_set_bg_opa(&cont_style, LV_OPA_100);
    lv_style_set_bg_color(&cont_style, lv_color_black());
    lv_style_set_radius(&cont_style, 0);
    lv_style_set_border_width(&cont_style, 0);

    charge_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(charge_cont, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_add_style(charge_cont, &cont_style, LV_PART_MAIN);
    lv_obj_center(charge_cont);

    lv_obj_add_event_cb(
        charge_cont, [](lv_event_t *e)
        { destoryChargeUI(); },
        LV_EVENT_PRESSED, NULL);

    int battery_percent = watch.getBatteryPercent();
    static int last_battery_percent = 0;

    lv_obj_t *arc = lv_arc_create(charge_cont);
    lv_obj_set_size(arc, LV_PCT(90), LV_PCT(90));
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_set_style_arc_color(arc, lv_color_make(19, 161, 14), LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);  /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE); /*To not allow adjusting by click*/
    lv_obj_center(arc);

    lv_obj_t *img_chg = lv_img_create(charge_cont);
    lv_obj_set_style_bg_opa(img_chg, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(img_chg, lv_color_make(19, 161, 14), LV_PART_ANY);

    lv_obj_t *label_percent = lv_label_create(charge_cont);
    lv_obj_set_style_text_font(label_percent, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_percent, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text_fmt(label_percent, "%d%%", battery_percent);

    // set user data
    lv_obj_set_user_data(arc, label_percent);
    lv_obj_set_user_data(label_percent, img_chg);

    lv_img_set_src(img_chg, &charge_done_battery);

    if (battery_percent == 100)
    {
        lv_arc_set_value(arc, 100);
        lv_img_set_src(img_chg, &charge_done_battery);
    }
    else
    {
        lv_img_set_src(img_chg, &img_usb_plug);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, arc);
        lv_anim_set_start_cb(&a, [](lv_anim_t *a)
                             {
            lv_obj_t *arc = (lv_obj_t *)a->var;
            lv_arc_set_value(arc, 0); });

        lv_anim_set_exec_cb(&a, charge_anim_cb);
        lv_anim_set_time(&a, 1000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, 500);
        lv_anim_set_values(&a, 0, 100);
        lv_anim_start(&a);
    }
    lv_obj_center(img_chg);
    lv_obj_align_to(label_percent, img_chg, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    lv_task_handler();
}

void destoryChargeUI()
{
    if (!charge_cont)
    {
        return;
    }
    lv_obj_del(charge_cont);
    charge_cont = NULL;
}

void PMUHandler()
{
    if (pmuIrq)
    {
        pmuIrq = false;
        watch.readPMU();
        if (watch.isVbusInsertIrq())
        {
            // //Serial.println("isVbusInsert");
            createChargeUI();
            watch.incrementalBrightness(brightnessLevel);
            usbPlugIn = true;
        }
        if (watch.isVbusRemoveIrq())
        {
            // //Serial.println("isVbusRemove");
            destoryChargeUI();
            watch.incrementalBrightness(brightnessLevel);
            usbPlugIn = false;
        }
        if (watch.isBatChagerDoneIrq())
        {
            // //Serial.println("isBatChagerDone");
        }
        if (watch.isBatChagerStartIrq())
        {
            // //Serial.println("isBatChagerStart");
        }
        // Clear watch Interrupt Status Register
        watch.clearPMU();
    }
}

void factory_ui()
{

    static lv_style_t bgStyle;
    lv_style_init(&bgStyle);
    lv_style_set_bg_color(&bgStyle, lv_color_black());
    lv_style_set_text_color(&bgStyle, lv_color_white());

    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_add_style(tileview, &bgStyle, LV_PART_MAIN);
    lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *t2 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    lv_obj_t *t4 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t4_1 = lv_tileview_add_tile(tileview, 1, 1, LV_DIR_TOP);
    lv_obj_t *t5 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
    analogclock(t2);

    radioPingPong(t4);
    radioSendAndRecievePage(t4_1);
    radioSendMessage(t5);
    lv_disp_trig_activity(NULL);

    lv_obj_set_tile(tileview, t2, LV_ANIM_OFF);
}

void settingButtonStyle()
{
    /*Init the button_default_style for the default state*/
    lv_style_init(&button_default_style);

    lv_style_set_radius(&button_default_style, 3);

    lv_style_set_bg_opa(&button_default_style, LV_OPA_100);
    lv_style_set_bg_color(&button_default_style, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_bg_grad_color(&button_default_style, lv_palette_darken(LV_PALETTE_YELLOW, 2));
    lv_style_set_bg_grad_dir(&button_default_style, LV_GRAD_DIR_VER);

    lv_style_set_border_opa(&button_default_style, LV_OPA_40);
    lv_style_set_border_width(&button_default_style, 2);
    lv_style_set_border_color(&button_default_style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&button_default_style, 8);
    lv_style_set_shadow_color(&button_default_style, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_shadow_ofs_y(&button_default_style, 8);

    lv_style_set_outline_opa(&button_default_style, LV_OPA_COVER);
    lv_style_set_outline_color(&button_default_style, lv_palette_main(LV_PALETTE_YELLOW));

    lv_style_set_text_color(&button_default_style, lv_color_white());
    lv_style_set_pad_all(&button_default_style, 10);

    /*Init the pressed button_default_style*/
    lv_style_init(&button_press_style);

    /*Add a large outline when pressed*/
    lv_style_set_outline_width(&button_press_style, 30);
    lv_style_set_outline_opa(&button_press_style, LV_OPA_TRANSP);

    lv_style_set_translate_y(&button_press_style, 5);
    lv_style_set_shadow_ofs_y(&button_press_style, 3);
    lv_style_set_bg_color(&button_press_style, lv_palette_darken(LV_PALETTE_YELLOW, 2));
    lv_style_set_bg_grad_color(&button_press_style, lv_palette_darken(LV_PALETTE_YELLOW, 4));

    /*Add a transition to the outline*/
    static lv_style_transition_dsc_t trans;
    static lv_style_prop_t props[] = {LV_STYLE_OUTLINE_WIDTH, LV_STYLE_OUTLINE_OPA, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&trans, props, lv_anim_path_linear, 300, 0, NULL);

    lv_style_set_transition(&button_press_style, &trans);
}
/*
 ************************************
 *      HARDWARE SETTING            *
 ************************************
 */
void setSportsFlag()
{
    sportsIrq = true;
}

void settingSensor()
{

    // Default 4G ,200HZ
    watch.configAccelerometer();

    watch.enableAccelerometer();

    watch.enablePedometer();

    watch.configInterrupt();

    watch.enableFeature(SensorBMA423::FEATURE_STEP_CNTR |
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

    watch.attachBMA(setSportsFlag);
}

void setPMUFlag()
{
    pmuIrq = true;
}

void settingPMU()
{
    watch.clearPMU();

    watch.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Enable the required interrupt function
    watch.enableIRQ(
        // XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |  // VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |     // POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ // CHARGE
        // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );
    watch.attachPMU(setPMUFlag);
}

lv_obj_t *message;

void updateTableDTP()
{

    String messageText = "Availible devices: \n";
    for (DTPNAPTimeRecord tableItem : DTP::getInstance()->neighbors())
    {
        // printf(("ID: " + String(tableItem.first) + " Routing ways: \n").c_str());
        messageText += "ID: " + String(tableItem.id) + " Routing ways: " + "\n";
        for (auto route : tableItem.routes)
        {
            messageText += "    " + String(route.first) + " distance: " + String(route.second.distance) + "\n";
        }
    }
    lv_label_set_text(message, NULL);

    lv_label_set_text(message, messageText.c_str());
}

/*
MAC::PacketReceivedCallback dataCallback = [](MACPacket *packet, uint16_t size, uint32_t crcCalculated)
{
    //Serial.println(String((char *)packet->data));
    String messageText = "#ffffff Recieved at:" + String(watch.strftime(1)) + " " + String(watch.getRSSI()) + " #ffffff " + String((char *)packet->data);
    //Serial.println(messageText);
    lv_label_set_text(message, NULL);

    lv_label_set_text(message, messageText.c_str());
    if (packet != NULL)
    {
        free(packet);
        packet = NULL;
    }
};
*/
LCMM::DataReceivedCallback lcmmDataCallback = [](LCMMPacketDataRecieve *packet, uint32_t size)
{
    // Perform actions with the received packet and size
    // For example, print the packet data to the console
    // Serial.println("Received packet from " + String(packet->mac.sender) + " to " + String(packet->mac.target) + " with packet type: " + String(packet->type) + ": \n");

    String messageText = " Recieved at:" + String(watch.strftime(1)) + " " + String(watch.getRSSI()) + " FROM:" + String(packet->mac.sender) + " Type: " + (packet->type == PACKET_TYPE_DATA_ACK ? "ACK" : "NOACK") + "data: " + String((char *)packet->data);

    // Serial.println(messageText);
    lv_label_set_text(message, NULL);

    lv_label_set_text(message, messageText.c_str());
    if (packet != NULL)
    {
        free(packet);
        packet = NULL;
    }
};

LCMM::AcknowledgmentCallback ackCallback = [](uint16_t packet, bool success)
{
    if (success)
    {
        String messageText = " Recieved at:" + String(watch.strftime(1)) + " " + String(watch.getRSSI()) + " " + "packet succesfully sent " + String(packet) + " " + "\n PING: " + String(LCMM::getInstance()->currentPing) + " \n";
        // Serial.println(messageText);
        lv_label_set_text(message, NULL);

        lv_label_set_text(message, messageText.c_str());
    }
    else
    {
        String messageText = " Recieved at:" + String(watch.strftime(1)) + " " + "packet failed to send " + String(packet);
        // Serial.println(messageText);
        lv_label_set_text(message, NULL);

        lv_label_set_text(message, messageText.c_str());
    }
};

static void sendmessage(lv_event_t *e)
{
    static bool sending = false;
    lv_event_code_t code = lv_event_get_code(e);
    // //Serial.println("event detected");
    if (!sending && code == LV_EVENT_CLICKED)
    {
        sending = true;
        // Serial.println("start sending");
        LCMM::getInstance()->sendPacketSingle(true, 2,
                                              (unsigned char *)"This is some important data",
                                              strlen("This is some important data") + 1, ackCallback);
        // Serial.println("end sending");

        sending = false;
    }
}

static void radioSendAndRecievePage(lv_obj_t *parent)
{
    // MAC::getInstance()->setRXCallback(dataCallback);
    DTP::initialize(35);

    lv_obj_t *label;
    lv_obj_t *sendbutton = lv_btn_create(parent);
    lv_obj_set_pos(sendbutton, 20, 15);
    lv_obj_add_event_cb(sendbutton, sendmessage, LV_EVENT_ALL, NULL);
    label = lv_label_create(sendbutton);
    lv_label_set_text(label, "Send message");
    message = lv_label_create(parent);
    lv_obj_set_width(message, 220);
    lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(message, true); /*Enable re-coloring by commands in the text*/
    lv_label_set_text(message, "#ffffff empty message");
    lv_obj_set_style_text_font(message, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_set_pos(message, 10, 50);

    lv_obj_center(label);
}

static lv_chart_series_t *ser1;

static void updateTheChart(lv_obj_t *chart)
{
    int minval = infinity();
    int maxval = -infinity();
    for (uint8_t i = 0; i < MAC::getInstance()->getNumberOfChannels(); i++)
    {
        int power = MAC::getInstance()->getNoiseFloorOfChannel(i);
        minval = (power < minval) ? power : minval;
        maxval = (power > maxval) ? power : maxval;
        ser1->y_points[i] = power;
        // //Serial.printf("noise floor of channel %d: %d\n", i, power);
    }
    maxval += 3;
    minval -= 3;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, minval, maxval);
    lv_chart_refresh(chart); /*Required after direct set*/
}
lv_obj_t *setupChart(lv_obj_t *obj)
{
    lv_obj_t *chart;
    chart = lv_chart_create(obj);
    lv_obj_align_to(chart, obj, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_size(chart, 170, 150);
    lv_chart_set_point_count(chart, MAC::getInstance()->getNumberOfChannels());
    lv_obj_center(chart);
    lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 6, 3, 9, 1, true, 40);
    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    updateTheChart(chart);

    return chart;
}
lv_obj_t *btn1;
static void scan_channels(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    // //Serial.println("event detected");
    if (code == LV_EVENT_CLICKED)
    {
        lv_color_t prevColor = lv_obj_get_style_bg_color(btn1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn1, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        lv_obj_refresh_style(btn1, LV_PART_ANY, LV_STYLE_PROP_ANY);
        MAC::getInstance()->LORANoiseCalibrateAllChannels(true);
        // //Serial.println("scanning done");
        lv_obj_t *chart = (lv_obj_t *)lv_event_get_user_data(e);
        updateTheChart(chart);

        lv_obj_set_style_bg_color(btn1, prevColor, LV_PART_MAIN);
        lv_obj_refresh_style(btn1, LV_PART_ANY, LV_STYLE_PROP_ANY);
    }
}

static void power_save(lv_event_t *e)
{
    static bool toggled = false;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        toggled = !toggled;
        if (toggled)
        {
            MAC::getInstance()->setMode(SLEEPING);
        }
        else
        {
            MAC::getInstance()->setMode(RECEIVING);
        }
    }
}
void radioPingPong(lv_obj_t *parent)
{
    lv_obj_t *next_parent = setupChart(parent);
    lv_obj_t *label;
    btn1 = lv_btn_create(parent);
    lv_obj_align_to(btn1, next_parent, LV_ALIGN_OUT_BOTTOM_MID, 45, 5);
    lv_obj_add_event_cb(btn1, scan_channels, LV_EVENT_ALL, next_parent);
    label = lv_label_create(btn1);
    lv_label_set_text(label, "Scan");
    lv_obj_center(label);

    lv_obj_t *btn2 = lv_btn_create(parent);
    lv_obj_add_event_cb(btn2, power_save, LV_EVENT_ALL, NULL);
    lv_obj_align_to(btn2, next_parent, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Power save");
    lv_obj_center(label);
}

uint16_t selected = -1;
lv_obj_t *dd;
lv_obj_t *responseMessage;
bool sending = false;
vector<uint16_t> devices;
static int checksum = 0;
void updateDropdown()
{
    int calculatedCheckSum = 0;
    int i = -1;
    for (auto tableItem : DTP::getInstance()->getRoutingTable())
    {
        calculatedCheckSum += tableItem.second.id*tableItem.second.distance * i;
        i *= -1 * tableItem.first;
    }

    if (calculatedCheckSum != checksum)
    {
        lv_dropdown_close(dd);
        lv_dropdown_clear_options(dd);

        devices.clear();

        for (auto tableItem : DTP::getInstance()->getRoutingTable())
        {
            char *text = (char *)malloc(32);
            devices.push_back(tableItem.first);
            snprintf(text, 32, "%d distance %d", tableItem.first, tableItem.second.distance);

            lv_dropdown_add_option(dd, text, LV_DROPDOWN_POS_LAST);
        }

        if(DTP::getInstance()->getRoutingTable().size() > 0){
            lv_dropdown_set_selected(dd, 0);
        }
    }
    checksum = calculatedCheckSum;
}
static void recievedAck(uint8_t result, uint16_t ping)
{
    sending = false;
    if (result)
    {
        printf("packet succeeded at getting into destination");
        String messageText = " Recieved at:" + String(watch.strftime(1)) + " " + String(watch.getRSSI()) + " " + "packet succesfully sent " + String(result) + " " + "\n PING: " + String(ping) + " \n";
        lv_label_set_text(responseMessage,  messageText.c_str());
    }
    else
    {
        printf("packet failed at getting to destination");
        String messageText = " Recieved at:" + String(watch.strftime(1)) + " " + "packet failed to send " + String(result);
        lv_label_set_text(responseMessage, messageText.c_str());
    }
}

static void sendDTPMessage(lv_event_t *e)
{
        lv_event_code_t code = lv_event_get_code(e);
    if (selected != 65535 && !sending && code == LV_EVENT_CLICKED)
    {
        sending = true;
        printf("selected message %d\n", selected);
        DTP::getInstance()->sendPacket((unsigned char *)"This is some important data", strlen("This is some important data") + 1, selected, 10000, recievedAck);
    }else if(code == LV_EVENT_CLICKED){
            lv_label_set_text(responseMessage, "Cannot send not valid value selected or already sending");

        printf("selected message %d sending %d\n", selected, sending);
    }
}

static void selected_device(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        selected = devices.at(lv_dropdown_get_selected(obj));

        printf("selected object: %d", selected);
    }
}

void radioSendMessage(lv_obj_t *parent)
{
    lv_obj_t *label;
    lv_obj_t *sendbutton = lv_btn_create(parent);
    lv_obj_set_pos(sendbutton, 20, 15);
    lv_obj_add_event_cb(sendbutton, sendDTPMessage, LV_EVENT_ALL, NULL);
    label = lv_label_create(sendbutton);
    lv_label_set_text(label, "Send message");
    responseMessage = lv_label_create(parent);
    lv_obj_set_width(responseMessage, 220);
    lv_label_set_long_mode(responseMessage, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(responseMessage, true); /*Enable re-coloring by commands in the text*/
    lv_label_set_text(responseMessage, "#ffffff empty message");
    lv_obj_set_style_text_font(responseMessage, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(responseMessage, 10, 50);
    lv_obj_center(label);

    /*Create a normal drop down list*/
    dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "No devices found yes\n");
    lv_dropdown_add_option(dd, "No devices found yes\n", 0);

    lv_obj_set_pos(dd, 10, 90);
    lv_obj_add_event_cb(dd, selected_device, LV_EVENT_ALL, NULL);
}

void analogclock(lv_obj_t *parent)
{
    bool antialias = true;
    lv_img_header_t header;

    const void *clock_filename = &clock_face;
    const void *hour_filename = &clock_hour_hand;
    const void *min_filename = &clock_minute_hand;
    const void *sec_filename = &clock_second_hand;

    lv_obj_t *clock_bg = lv_img_create(parent);
    lv_img_set_src(clock_bg, clock_filename);
    lv_obj_set_size(clock_bg, 240, 240);
    lv_obj_center(clock_bg);

    hour_img = lv_img_create(parent);
    lv_img_decoder_get_info(hour_filename, &header);
    lv_img_set_src(hour_img, hour_filename);
    lv_obj_center(hour_img);
    lv_img_set_pivot(hour_img, header.w / 2, header.h / 2);
    lv_img_set_antialias(hour_img, antialias);

    lv_img_decoder_get_info(min_filename, &header);
    min_img = lv_img_create(parent);
    lv_img_set_src(min_img, min_filename);
    lv_obj_center(min_img);
    lv_img_set_pivot(min_img, header.w / 2, header.h / 2);
    lv_img_set_antialias(min_img, antialias);

    lv_img_decoder_get_info(sec_filename, &header);
    sec_img = lv_img_create(parent);
    lv_img_set_src(sec_img, sec_filename);
    lv_obj_center(sec_img);
    lv_img_set_pivot(sec_img, header.w / 2, header.h / 2);
    lv_img_set_antialias(sec_img, antialias);

    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_color(&label_style, lv_color_white());

    battery_percent = lv_label_create(parent);
    lv_label_set_text(battery_percent, "100");
    lv_obj_align(battery_percent, LV_ALIGN_LEFT_MID, 68, -10);
    lv_obj_add_style(battery_percent, &label_style, LV_PART_MAIN);

    weather_celsius = lv_label_create(parent);
    lv_label_set_text(weather_celsius, "23°C");
    lv_obj_align(weather_celsius, LV_ALIGN_RIGHT_MID, -62, -2);
    lv_obj_add_style(weather_celsius, &label_style, LV_PART_MAIN);

    step_counter = lv_label_create(parent);
    lv_label_set_text(step_counter, "6688");
    lv_obj_align(step_counter, LV_ALIGN_BOTTOM_MID, 0, -55);
    lv_obj_add_style(step_counter, &label_style, LV_PART_MAIN);

    clockTimer = lv_timer_create([](lv_timer_t *timer)
                                 {
                                    /*if(time_ready){
                                        if(wifi_turned_on){
                                            WiFi.mode(WIFI_MODE_NULL);
                                            wifi_turned_on = false;
                                        }*/

                                    struct tm timeinfo;
                                    watch.getDateTime(&timeinfo);
                                    
                                    ////Serial.println(watch.strftime());
                                    lv_img_set_angle(
                                        hour_img, ((timeinfo.tm_hour) * 300 + ((timeinfo.tm_min) * 5)) % 3600);
                                    lv_img_set_angle(min_img, (timeinfo.tm_min) * 60);

                                    lv_anim_t a;
                                    lv_anim_init(&a);
                                    lv_anim_set_var(&a, sec_img);
                                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
                                    lv_anim_set_values(&a, (timeinfo.tm_sec * 60) % 3600,
                                                        (timeinfo.tm_sec + 1) * 60);
                                    lv_anim_set_time(&a, 1000);
                                    lv_anim_start(&a);
                                    //}

                                     // Update step counter
                                     lv_label_set_text_fmt(step_counter, "%u", stepCounter);

                                     // Update battery percent
                                     int percent = watch.getBatteryPercent();
                                     lv_label_set_text_fmt(battery_percent, "%d", percent == -1 ? 0 : percent);

                                    float  temp = watch.readAccelTemp();
                                    ////Serial.print(temp);
                                    ////Serial.println("*C");
                                    lv_label_set_text_fmt(weather_celsius, "%d°C", (int)temp); },
                                 1000, NULL);
}
