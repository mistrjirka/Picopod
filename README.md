# Picopod T-Watch S3

Picopod firmware for the LilyGo T-Watch S3, using DTProtocol v3 over the
on-board SX1262 and a BLE gateway for phone/laptop access.

## Watch UI

Swipe horizontally between four pages:

1. **Home** — time/date, battery or USB state, BLE state, steps, sensor
   temperature, route count and the latest message or delivery event.
2. **Messages** — select a reachable node and send one of five useful quick
   messages. Arbitrary text is available through the BLE client in DTProtocol.
3. **Mesh** — complete reachable-node table, next hop, distance, manual refresh
   and the startup-calibrated channel-noise range without retuning the live radio.
4. **Settings** — brightness, screen timeout, message vibration, immediate
   screen-off and live radio/BLE diagnostic counters.

A short power-button press toggles the display. A long press saves pending
settings and powers down. Touch, double-tap motion and incoming messages wake the
screen. Turning the screen off does **not** suspend LoRa or BLE.

## Reliability and ownership

- All LVGL calls run in the Arduino loop; radio/BLE callbacks post bounded events.
- The LVGL draw-buffer size is passed in pixels, while PSRAM allocation uses
  bytes, preventing the old RGB565 buffer overrun.
- Settings writes are debounced to reduce flash wear.
- Wi-Fi and unused audio/IR/FFT libraries are removed.
- Filesystem mounting/automatic formatting is removed because the UI has no
  filesystem assets.
- The BLE bridge honors the negotiated ATT MTU, fragments lazily and serializes
  DTProtocol access through the main loop.

## Radio profile

The watch and Heltec node use the same private LoRa PHY:

```text
EU868 channel 0 (868.1 MHz), SF9, BW125, CR 4/7, private sync word
```

Node ID is 3. The persistent DTPK boot sequence advances once per reboot.

## Build

```bash
pio run -e twatch-s3
```
