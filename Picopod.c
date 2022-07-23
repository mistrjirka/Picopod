#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lib/voltage/voltage.h"
#include "string.h"
#include "lib/lora/LoRa-RP2040.h"

#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11



int main()
{
    stdio_init_all();

    initVoltage(26, 2);
    float voltage;
    while (true){
        voltage = getVoltage();
        printf(" voltage: %f \n", voltage);
        sleep_ms(1000);
    }
    return 0;
}
