#include "watch/setup/watch_setup.h"
#include <WiFi.h>
#ifdef ENABLE_IR_SENDER
#include <IRsend.h>
IRsend irsend(BOARD_IR_PIN);

#endif
#include <driver/i2s.h>
#include <esp_vad.h>

void setup()
{

    watchSetup();
}

void loop()
{

    SensorHandler();

    PMUHandler();

    bool screenTimeout = lv_disp_get_inactive_time(NULL) < DEFAULT_SCREEN_TIMEOUT;
    if (screenTimeout ||
        !canScreenOff ||
        usbPlugIn)
    {
        if (!screenTimeout)
        {
            if (usbPlugIn &&
                (pageId != RADIO_TRANSMIT_PAGE_ID))
            {
                createChargeUI();
            }
            lv_disp_trig_activity(NULL);
        }
        lv_task_handler();
        delay(2);
    }
    else
    {
        lowPowerEnergyHandler();
    }
}