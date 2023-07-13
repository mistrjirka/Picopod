#ifndef LORA_CALCULATOR_H
#define LORA_CALCULATOR_H

class LoRaCalculator {
public:
    static double calculateTime(int payload_len);

private:
    static double calculateNPayload(int payload_len, int spreading_factor, bool crc, bool explicit_header, bool low_data_rate_opt);
};

#endif
