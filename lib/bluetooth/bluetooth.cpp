#include <stdio.h>
#include "pico/stdlib.h"
#include "bluetooth.h"

void BluetoothClass::BluetoothSend(char *message)
{
    uart_puts(UART_ID, message);
}

irq_handler_t BluetoothClass::BluetoothUARTRX()
{
    printf("nice");
    char *message;
    BluetoothSend("nice");
    while (uart_is_readable(UART_ID))
    {
        uint8_t ch = uart_getc(UART_ID);
        printf("%c", ch);
        message += ch;

        BL_chars_rxed++;
    }
    printf(message);
}

void BluetoothClass::BluetoothSetup(irq_handler_t bluetoothUartrx)
{
    uart_init(UART_ID, UART_BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, bluetoothUartrx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}
BluetoothClass Bluetooth;