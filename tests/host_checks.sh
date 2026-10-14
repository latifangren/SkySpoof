#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CONFIG="$ROOT/module/config.json"
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
python3 - "$CONFIG" <<'PY'
import json, sys
p=sys.argv[1]
with open(p, encoding='utf-8') as f: json.load(f)
print('JSON syntax OK')
PY
"$ROOT/build/skyspoofctl-host" validate "$CONFIG"
for abi in arm64-v8a armeabi-v7a x86_64; do
  test -f "$ROOT/build/$abi/libskyspoof.so"
done
printf '%s\n' 'host checks OK'
