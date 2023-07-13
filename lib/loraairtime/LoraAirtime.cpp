#include "LoraAirtime.h"
#include <cmath>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "../lora/LoRa-RP2040.h"

double LoRaCalculator::calculateTime(int payload_len) {
    int spreading_factor = LoRa.getSpreadingFactor();
    long bandwidth = LoRa.getSignalBandwidth();
    int coding_rate = LoRa.getCodingRate4();
    bool crc = LoRa.getCrc();
    bool explicit_header = LoRa.getExplicitHeaderMode();
    bool low_data_rate_opt = LoRa.getLowDataRateOptimize();

    double symbol_time = pow(2, spreading_factor) / bandwidth;
    double n_preamble = 4.25;
    double t_preamble = n_preamble * symbol_time;
    double n_payload = calculateNPayload(payload_len, spreading_factor, crc, explicit_header, low_data_rate_opt);
    double t_payload = n_payload * symbol_time;

    return t_preamble + t_payload;
}

double LoRaCalculator::calculateNPayload(int payload_len, int spreading_factor, bool crc, bool explicit_header, bool low_data_rate_opt) {
    double payload_bit = 8 * payload_len;
    payload_bit -= 4 * spreading_factor;
    payload_bit += 8;
    payload_bit += crc ? 16 : 0;
    payload_bit += explicit_header ? 20 : 0;
    payload_bit = std::max(payload_bit, 0.0);
    double bits_per_symbol = low_data_rate_opt ? spreading_factor - 2 : spreading_factor;
    double payload_symbol = std::ceil(payload_bit / 4 / bits_per_symbol) * 5;  // Assuming coding rate is always 5

    return payload_symbol;
}
