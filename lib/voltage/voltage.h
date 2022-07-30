#pragma once
class VoltageClass
{
private:
    int voltage_divider = 2;
    int pin = 26;
    int pin_adc = 0;
    int calc_adcin(int pin);

public:
    void initVoltage(int pin, int divider);
    float getVoltage(void);
};
extern VoltageClass Voltage;
