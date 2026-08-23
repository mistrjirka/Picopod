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

Run `./flash-ui.sh` (or the Python command above), then open
`http://127.0.0.1:8765/`. Select the exact device, enter a unique node ID, choose
an enumerated serial port or Auto-detect, optionally enable debug echo, then use
**Build only** or **Build and flash**. The Cancel button terminates the active
PlatformIO process group.

The tool has no web dependencies and never binds to the LAN. Builds happen in
`~/.cache/picopod-flasher`, using fresh production branches and their pinned
DTProtocol submodule. The current checkout is not switched or edited. A
cache-wide file lock prevents two separately launched flasher processes from
racing on generated configuration or one physical upload port. Generated node
configuration is deleted after every success, failure, or cancellation, so it
cannot leak into the next build.

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
limited to 64 bytes. A protocol marker prevents echo loops even when two debug
nodes use different suffixes. Echo replies request an end-to-end ACK and report
final success or failure on the serial console; retries keep one application
identity, so the receiver still exposes the reply only once.

## Troubleshooting

- No serial port: reconnect the board, check `ls -l /dev/ttyACM* /dev/ttyUSB*`,
  and verify `id -nG` contains `uucp` after logging in again.
- RP2040 upload failure: hold BOOTSEL while connecting USB, then retry with
  Auto-detect.
- A failed or cancelled build can be restarted immediately. Use **Clean build**
  if a toolchain or dependency changed.
- The browser shows a bounded live log and the exact production commit being
  built. User-provided suffix text is passed only through environment variables,
  never as shell syntax.
