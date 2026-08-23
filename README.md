# Picopod Heltec Wireless Stick Lite V3

Headless Picopod node for the Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262).
It runs DTProtocol v3 and exposes the same BLE gateway as the watch.

## Hardware

The radio pins are explicit in `src/main.cpp`:

| Signal | GPIO |
|---|---:|
| NSS | 8 |
| SCK | 9 |
| MOSI | 10 |
| MISO | 11 |
| reset | 12 |
| BUSY | 13 |
| DIO1 | 14 |

A local PlatformIO board definition names the actual product while reusing the
compatible Heltec ESP32-S3 Arduino variant. Startup first tries the usual 1.8 V
DIO3 TCXO configuration, then the externally powered oscillator mode so board
revisions fail visibly rather than hanging forever.

## Radio profile

The Heltec node matches the T-Watch exactly:

```text
EU868 channel 0 (868.1 MHz), SF9, BW125, CR 4/7, private sync word
```

Node ID is 2. The DTPK boot incarnation is persisted in ESP32 Preferences.

## Build

```bash
pio run -e heltec_wireless_stick_lite_v3
```

The serial console at 115200 baud reports routes, radio status, BLE MTU and queue
drop counters. Fatal startup errors are logged and followed by a controlled
restart instead of an infinite silent loop.
