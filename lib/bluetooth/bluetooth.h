#pragma once
#include <stdio.h>
#include "bluetooth.h"
#include "pico/stdlib.h"

/*bluetooth modul*/
#define UART_ID uart0
#define UART_BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

class BluetoothClass
{
private:
    static int BL_chars_rxed;
    irq_handler_t BluetoothUARTRX();

public:
    void BluetoothSetup(irq_handler_t bluetoothUartrx);
    void BluetoothSend(char *message);
};
extern class BluetoothClass Bluetooth;
