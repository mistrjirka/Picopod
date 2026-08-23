# Picopod

Picopod is a small LoRa mesh project with production firmware for the LilyGo
T-Watch S3, Heltec Wireless Stick Lite V3, and a custom RP2040 board.

## Build and flash from Arch Linux

The local browser UI selects the device type, node ID, upload port, and optional
debug echo mode:

```bash
sudo pacman -S --needed git python
python -m venv .venv
.venv/bin/pip install --upgrade platformio
./flash-ui.sh
```

See [FLASHING_UI.md](FLASHING_UI.md) for device-specific details. Production
firmware is maintained in the `espwatch`, `heltec-stick-lite-v3`, and
`refactor-to-radiolib` branches. The flasher fetches and builds those branches in
an isolated cache, so this checkout is not switched or modified.
