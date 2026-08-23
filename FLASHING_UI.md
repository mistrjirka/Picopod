# Picopod flashing UI

The local browser UI builds and flashes all current Picopod hardware variants:

- LilyGo T-Watch S3;
- Heltec Wireless Stick Lite V3;
- the custom RP2040 Picopod board.

It asks for a device type and a unique node ID. An optional **debug echo node**
mode repeats each received application message back to its original sender with
a configurable suffix. DTProtocol marks the returned packet as a debug echo, so
another debug node never repeats it even when the two nodes use different
suffixes. Matching-suffix detection remains an additional compatibility guard.

## Arch Linux setup

```bash
sudo pacman -S --needed git python
python -m venv .venv
.venv/bin/pip install --upgrade platformio
.venv/bin/python tools/picopod_flash_ui.py
```

The tool opens `http://127.0.0.1:8765/`. It has no web dependencies and never
binds to the LAN. Builds happen in `~/.cache/picopod-flasher`, using fresh
production branches and their pinned DTProtocol submodule. The current checkout
is not switched or edited. A cache-wide file lock also prevents two separately
launched flasher processes from racing on the generated node configuration or
one physical upload port.

For serial access on Arch, the user normally needs membership in `uucp`:

```bash
sudo usermod -aG uucp "$USER"
```

Log out and back in after changing groups. The RP2040 may need to be connected in
BOOTSEL mode. PlatformIO auto-detects upload ports when **Auto-detect** is
selected; an explicit port can be chosen when multiple boards are attached.

## Build-time configuration

The UI supplies these environment variables to PlatformIO:

```text
PICOPOD_NODE_ID=1..65535
PICOPOD_DEBUG_ECHO=0|1
PICOPOD_DEBUG_ECHO_SUFFIX=" [echo]"
```

The same variables can be used without the UI:

```bash
PICOPOD_NODE_ID=42 \
PICOPOD_DEBUG_ECHO=1 \
PICOPOD_DEBUG_ECHO_SUFFIX=' [lab echo]' \
pio run -e twatch-s3 -t upload
```

Node ID `0` is reserved for broadcast. The suffix is encoded as UTF-8 and is
limited to 64 bytes. Debug echo uses reliable per-hop delivery but does not ask
for a separate end-to-end ACK, avoiding an unnecessary application-transaction
stall on a diagnostic node.
