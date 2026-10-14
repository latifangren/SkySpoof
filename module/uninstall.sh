#!/system/bin/sh
DATA=/data/adb
CONFIG=$DATA/SkySpoof.json
BACKUP=$DATA/SkySpoof.backup.json
DISABLE=$DATA/SkySpoof.disabled
if [ -f "$CONFIG" ]; then
  cp "$CONFIG" "$BACKUP" 2>/dev/null || true
  chmod 600 "$BACKUP" 2>/dev/null || true
fi
rm -f "$CONFIG" "$DISABLE"
