# Picopod custom RP2040 node

This target restores the third Picopod hardware variant. Repository history
shows that it is a **Raspberry Pi Pico / RP2040**, not an ESP8266 or ESP32. The
Pico module sits on the custom Picopod PCB with an LLCC68/SX126x-compatible LoRa
radio.

## Recovered board wiring

| Function | RP2040 GPIO |
|---|---:|
| SPI1 MISO | 12 |
| SPI1 MOSI | 11 |
| SPI1 SCK | 10 |
| Radio CS/NSS | 13 |
| Radio DIO1 | 9 |
| Radio reset | 14 |
| Radio BUSY | 19 |
| Status LED | 25 |

The latest historical target used EU433 channel 2 (`433.425 MHz`), SF8,
125 kHz bandwidth and coding rate 4/7. Those settings are preserved so the port
matches the original custom-board network rather than the separate EU868 watch
and Heltec network.

## Build

```bash
pio run -e picopod_rp2040
```

The PlatformIO environment pins the RP2040 platform fork at
`9c167c6b8aac4f4cfa6d55a0c4e5b848795150c0`, which resolves Arduino-Pico 6.0.0
at `9a0bc35654e9af3eccfb88c54f9cc73f9e153ac6`. RadioLib stays at the
DTProtocol-tested 6.6.0 release, and the exact DTProtocol v3 revision is a Git
submodule.

## Serial console

At 115200 baud:

```text
status
routes
send <node> <text>
reboot
help
```

The firmware persists the DTPK boot incarnation once per boot in emulated EEPROM,
retries radio initialization, restarts after fatal startup failures and exposes
radio/heap diagnostics instead of hanging silently.
