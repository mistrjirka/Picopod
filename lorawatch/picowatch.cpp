#include <Arduino.h>
#include "watch/setup/watch_setup.h"

void setup()
{
    Serial.begin(115200);
    delay(100);
    if (!watchSetup())
    {
        Serial.println("Picopod watch initialization failed; restarting");
        delay(3000);
        ESP.restart();
    }
}

void loop()
{
    watchLoop();
}
