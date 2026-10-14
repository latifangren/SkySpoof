#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd)
NDK=${ANDROID_NDK_HOME:-/opt/android-sdk/ndk/27.0.12077973}
TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
OUT="$ROOT/build"
ZIP="$ROOT/SkySpoof-0.3.0.zip"
rm -rf "$OUT" "$ROOT/package" "$ZIP"
mkdir -p "$OUT"
${CXX:-clang++} -std=c++17 -O2 -I "$ROOT/zygisk" \
  "$ROOT/zygisk/configctl.cpp" -o "$OUT/skyspoofctl-host"
for abi in arm64-v8a armeabi-v7a x86_64; do
  cmake -S "$ROOT/zygisk" -B "$OUT/$abi" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DANDROID_ABI="$abi" \
    -DANDROID_PLATFORM=android-31 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
  cmake --build "$OUT/$abi"
done
mkdir -p "$ROOT/package/zygisk" "$ROOT/package/META-INF/com/google/android"
cp "$ROOT/module"/module.prop "$ROOT/module"/config.json "$ROOT/module"/config.schema.json \
   "$ROOT/module"/devices.conf \
   "$ROOT/module"/customize.sh "$ROOT/module"/service.sh "$ROOT/module"/uninstall.sh "$ROOT/package/"
cp -r "$ROOT/module/webroot" "$ROOT/package/webroot"
cp "$ROOT/module/META-INF/com/google/android"/* "$ROOT/package/META-INF/com/google/android/"
cp "$OUT/arm64-v8a/libskyspoof.so" "$ROOT/package/zygisk/arm64-v8a.so"
cp "$OUT/armeabi-v7a/libskyspoof.so" "$ROOT/package/zygisk/armeabi-v7a.so"
cp "$OUT/x86_64/libskyspoof.so" "$ROOT/package/zygisk/x86_64.so"
cp "$OUT/arm64-v8a/skyspoofctl" "$ROOT/package/skyspoofctl_arm64"
cp "$OUT/armeabi-v7a/skyspoofctl" "$ROOT/package/skyspoofctl_armv7"
cp "$OUT/x86_64/skyspoofctl" "$ROOT/package/skyspoofctl_x86_64"
chmod 0755 "$ROOT/package"/*.sh "$ROOT/package/META-INF/com/google/android/update-binary" "$ROOT/package"/skyspoofctl_*
( cd "$ROOT/package" && zip -qr "$ZIP" . )
unzip -t "$ZIP" >/dev/null
printf 'Built %s\n' "$ZIP"
