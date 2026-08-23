#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if [[ -x "$ROOT/.venv/bin/python" ]]; then
  exec "$ROOT/.venv/bin/python" "$ROOT/tools/picopod_flash_ui.py" "$@"
fi
exec python3 "$ROOT/tools/picopod_flash_ui.py" "$@"
