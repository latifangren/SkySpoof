#!/system/bin/sh
MODDIR=${0%/*}
DATA=/data/adb
CONFIG=$DATA/SkySpoof.json
BACKUP=$DATA/SkySpoof.backup.json
DISABLE=$DATA/SkySpoof.disabled
case "$(uname -m)" in
  aarch64) VALIDATOR=$MODDIR/skyspoofctl_arm64 ;;
  armv7l|armv8l) VALIDATOR=$MODDIR/skyspoofctl_armv7 ;;
  x86_64) VALIDATOR=$MODDIR/skyspoofctl_x86_64 ;;
  *) VALIDATOR= ;;
esac

mkdir -p "$DATA"
if [ ! -f "$CONFIG" ]; then
  cp "$MODDIR/config.json" "$CONFIG" 2>/dev/null || exit 0
  chmod 600 "$CONFIG" 2>/dev/null || true
fi

if [ -f "$DISABLE" ]; then
  log -t SkySpoof "disabled marker present; leaving module inactive"
  exit 0
fi

if [ -x "$VALIDATOR" ]; then
  if ! "$VALIDATOR" validate "$CONFIG" >/dev/null 2>&1; then
    if [ -f "$BACKUP" ] && "$VALIDATOR" validate "$BACKUP" >/dev/null 2>&1; then
      cp "$BACKUP" "$CONFIG"
      chmod 600 "$CONFIG" 2>/dev/null || true
      log -t SkySpoof "invalid config restored from backup"
    else
      touch "$DISABLE"
      log -t SkySpoof "invalid config and no valid backup; disabled"
    fi
  fi
fi
chmod 600 "$CONFIG" 2>/dev/null || true
