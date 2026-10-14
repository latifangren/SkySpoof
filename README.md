# SkySpoof

Original open-source Zygisk device-profile module.

## Version 0.3.0

- Documentation: [`docs/REA-ANALYSIS.md`](docs/REA-ANALYSIS.md)
- Device catalog imported from DeviceSpoofLab-Magisk (`module/devices.conf`)
- Offline WebUI profile editor in `webroot/`
- Catalog presets for Pixel 7/8/9 Pro
- Multi-profile and package mapping
- Native config validator with strict allowlist
- Invalid-config backup recovery and disable marker
- Build / Build.VERSION compatibility overrides
- ABI builds: arm64-v8a, armeabi-v7a, x86_64

The WebUI is a static editor that downloads `SkySpoof.json`; installation and activation remain explicit root-module operations.

## Explicit limits

This project does not bypass Widevine keyboxes, Play Integrity attestation, IMEI restrictions, anti-cheat, or third-party licenses.

## Build

```bash
./build.sh
```

Artifact:

```text
SkySpoof-0.3.0.zip
```
