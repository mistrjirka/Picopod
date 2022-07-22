#include <stdio.h>
#include "pico/stdlib.h"
#include "voltage.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

int voltage_divider = 2;
int pin = 26;
int pin_adc = 0;

int calc_adcin(int pin)
{
    switch (pin)
    {
    case 26:
        return 0;
        break;
    case 27:
        return 1;
        break;
    case 28:
        return 2;
        break;
    case 29:
        return 3;
        break;

    default:
        return false;
        break;
    }
}

float getVoltage(void)
{
    //adc_select_input(pin_adc);
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float compensated_result = result * conversion_factor;
    float acctual_voltage = compensated_result * voltage_divider;
    //printf("acctual voltage: %f, raw: %f ", acctual_voltage, result * conversion_factor);
    return acctual_voltage;

}
void initVoltage(int pin_par, int divider_par)
{
    adc_init();
    pin = pin_par;
    voltage_divider = divider_par;
    pin_adc = calc_adcin(pin);
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(pin);
    adc_select_input(pin_adc);
}
