# REA Analysis: COPG Pro and COPG-VD

Status: static analysis documentation

This document records the reverse-engineering observations made with REA/Ghidra against the public COPG v7.5.1 release and the public COPG-VD fork. It is evidence-led: a string or README claim is not treated as proof of a working runtime feature.

## Scope and boundaries

- Target: public Android root-module artifacts and public source files.
- Analysis type: file inventory, ELF inspection, string/xref triage, native procedure inspection, and DEX decompilation of plaintext helper DEX files.
- No Android device was connected during the documented work. Runtime behavior, hook timing, and device compatibility are therefore not confirmed.
- No license bypass, key extraction, payload patching, binary modification, or attestation bypass was performed.

## Evidence levels

- **Confirmed static evidence:** directly observed in a binary/source file, decompiled helper, symbol, import, string, or reproducible hash.
- **Static indication:** a related string, manifest, resource, or import suggests a capability, but does not prove that the capability is active or complete.
- **Project claim:** feature described by README, UI, or metadata without independent implementation/runtime confirmation.
- **Not established:** the available evidence is insufficient to make the claim.

## COPG Pro v7.5.1 artifacts

The analyzed release was `COPG-v7.5.1.zip`.

| Artifact | SHA-256 | Observation |
|---|---|---|
| `zygisk/arm64-v8a.so` | `0a85a837ce61a779a2281ae9102c3e62a4cc5da804e42676693f96ff7498595d` | Main arm64 Zygisk shared object |
| `copg_lic_arm64` | `86387f8152ab5aa76eb770ad5be6553d54c453fb4d82523b194912255c451c6e` | UPX-compressed arm64 licensing/enrollment helper |
| `wv.dex` | `4c7431592b426a0eabc3ed0d818a48173352daa2367ade64fc47972a8eb66e5b` | Plaintext Widevine helper DEX |
| `userprobe.dex` | `8de7ba4d571998af5417e3c67c70e85668245db2a324d69cc9f1c47d187416d3` | Plaintext user/package probe DEX |

These hashes identify the analyzed files. They do not prove that another downloaded release has identical contents.

## COPG Pro native loader

### Confirmed static observations

The arm64 Zygisk object contains the following analyzed procedure entry points:

```text
zygisk_module_entry     0xac17f0
zygisk_companion_entry  0xae90c0
```

The object also contains a `dlopen` import/thunk, showing that dynamic library loading is part of the native component.

The following strings are present in the native object:

```text
InMemoryDexClassLoader
/data/adb/modules/COPG/license/license.bin
[LIC] paid dex decrypt failed (no/invalid K_GPU)
```

The strongest supported model is:

```text
Zygisk native loader
  -> license/device validation
  -> paid-Dex decryption gate
  -> InMemoryDexClassLoader path
  -> Java hook loading
```

The arrows describe an evidence-based architecture hypothesis. The exact order and complete control flow were not established by a complete runtime trace.

### Licensing helper

`copg_lic_arm64` is an arm64 PIE executable and contains strings for:

```text
enroll
status
fp
COPG-enroll
state=%s
buyer=0x%llx
tier=%u
expiry=%u
/data/adb/modules/COPG/license/license.bin
runhev: no license
runhev: license not valid
```

This confirms an enrollment/status/fingerprint interface and license-state reporting in the helper. It does **not** disclose the license key, signature material, or decryption key.

## COPG Pro feature map

### GPU

**Confirmed static evidence:**

```text
libEGL.so
libGLESv1_CM.so
libGLESv2.so
libvulkan.so
eglQueryString
/data/adb/modules/COPG/GPU/manifest.json
```

The release manifest includes profiles named `adreno830` and `adreno840`, with renderer/vendor values for Qualcomm Adreno hardware.

**Conclusion:** OpenGL renderer/vendor spoofing is strongly supported by static evidence. Vulkan handling is indicated by the Vulkan library reference, but its precise hook behavior is not established.

### IMEI, SIM, and carrier

**Static evidence includes:**

```text
com.copg.imei.ImeiHook
com.copg.imeiapp.ImeiHook
com.copg.imeiapp.SimOpHook
```

Native log strings expose parameters resembling:

```text
ImeiHook.installWith(imei, imsi, iccid)
SimOpHook.installWith(numeric, iso, name)
```

**Conclusion:** these hook components are present or expected by the loader. Runtime success, modem-service coverage, and device/API compatibility were not tested.

### GPS and cell tower

**Static evidence includes:**

```text
com.copg.gps.GpsHook
com.copg.cell.CellHook
GpsHook.installWith(lat, lng)
CellHook.installWith(lac, ci, mcc, mnc)
```

**Conclusion:** GPS and cell-related hook entry points are indicated. No runtime behavior was verified.

### DRM / Widevine

The plaintext `wv.dex` decompiles to class `WV`. Its `main` method creates an Android `MediaDrm` instance using the Widevine UUID and reads:

```text
deviceUniqueId
vendor
version
securityLevel
```

The native object also references:

```text
com.copg.drm.DrmHook
DrmHook.installWith(level, systemId, deviceUniqueId)
```

**Conclusion:** COPG contains a Widevine property probe and a DRM hook entry point. This does not prove that hardware-backed L1 credentials or a keybox can be replicated.

### Google services and account-related identifiers

**Static evidence includes:**

```text
com.copg.gms.GmsHook
com.copg.gmsacct.AccountGmsHook
GmsHook.installGaid(id)
GmsHook.installAppset(id, scope)
```

**Conclusion:** Advertising ID and App Set ID handling is indicated. Account-related hook code is indicated by class names, but implementation and runtime coverage are not established.

### Remaining Java hook names

The native binary references the following additional components:

```text
com.copg.playsrc.PlaysrcHook
com.copg.sensor.SensorHook
com.copg.tensorfeat.TensorfeatHook
com.copg.wallpaper.WallpaperHook
com.copg.webua.WebUaHook
com.copg.mock.MockHook
```

These names are static indications. They should not be written as proof that every feature works on every Android version.

## Plaintext helper DEX files

### `wv.dex`

Purpose supported by decompilation: probe Widevine `MediaDrm` properties and print results. It is not the encrypted paid feature payload.

### `userprobe.dex`

Decompiled class:

```text
com.copg.userprobe.Main
```

Observed responsibilities:

- enumerate Android users;
- query installed packages per user;
- identify system packages;
- return labels;
- render package icons to PNG;
- use `IUserManager`, `IPackageManager`, `LauncherApps`, and related framework APIs.

This helper supports app/user selection in the WebUI. It is not evidence of the paid hook implementation.

## COPG-VD fork

Repository: `https://github.com/VD171/COPG-VD`

### Confirmed source-level characteristics

- Global device spoofing through a Zygisk C++ module.
- Main configuration path: `/data/adb/COPG-VD.json`.
- Build and `Build.VERSION` fields are written from the JSON profile.
- The code includes version-policy logic intended to avoid publishing an SDK/framework combination newer than the actual ROM unless explicitly forced.
- The module uses `DLCLOSE_MODULE_LIBRARY` after loading.
- The repository contains source files such as `zygisk/spoof_module.cpp`, `zygisk/zygisk.hpp`, and `zygisk/json.hpp`.

### Difference from COPG Pro

COPG-VD is not a decrypted or unlocked COPG Pro build. Its public source is a separate global Build/profile spoofing implementation. The analyzed fork does not contain the COPG Pro release's encrypted paid DEX, `copg_lic`, or the corresponding licensing directory.

## Relationship to SkySpoof

SkySpoof uses the useful, auditable ideas from these projects without copying proprietary payloads:

- explicit per-package profile mapping;
- device catalog entries based on build fingerprints;
- strict field allowlists;
- offline profile editor;
- host-side validation;
- backup/recovery behavior;
- app-process-only scope.

SkySpoof intentionally does not implement license bypass, Widevine keybox changes, hardware attestation bypass, anti-cheat evasion, or telecom identity fabrication.

## REA evidence references

Evidence records retained during the session include:

```text
ev_1fd3e4900ffec646eabbf024bf33409521772690f16f8d3163954f7a0c0ccf99
 ev_dc834bdf23b51fb691f69e75c2fda45580569ced0582369b880a2ed0c81f839e
 ev_6586f47e8314bd0a7c1f57a524cfa4aab70ca4851e8635919c52ce8df475c70d
 ev_9d3e7be65c3b8004692583f9717e41cb518adcb9621c146376a5928a7cada125
 ev_3f9848a48ea4749bdc068d8c020cec69ff110b56a8c14356460c7a7a1e47b662
 ev_3035ac09b8d1018133facfb66b40f5d1735e9d3afa4458dddfa13592be5adf48
 ev_e934a9897a5be8959005737013148ec808c1d1d078656636e34b8066fc78a64f
 ev_c4774c87ce2592b84a3c05b2fffe1c5ed8eb9c02188c00e4a738fb1025f05ec0
 ev_fc83ae6edd1190212a9e9ab2050e77c7e9e856f4eead199b119bee740e6d1594
 ev_39168bc9058fd9362dbcfd9c85048873b40f4178da0f384c8dab6b87d3f90fc9
 ev_9eea99de859d09ab07b93edb1227e74be767348a1e7eed435a2afc5bc9347113
 ev_43eb6627446b25aa18e63a5df5bf95ead9cb4253023a642e30056d0fcce05f85
```

The IDs are REA evidence handles from the analysis session. The binary SHA-256 values above remain the primary artifact identity checks.

## REA limitations

- The REA provider changed generated document identities during the session; document names must be treated as provider-generated, not filenames.
- Some provider calls entered cooldown after invalid document targeting. This limited a complete call-graph/decompiler pass.
- The paid Java payload was not recovered as plaintext.
- The decryption algorithm, key derivation, signature scheme, tier bitmask, and full loader call path are not established.
- No device/emulator was connected, so no claims of runtime success are made.
- Static strings can be dead code, error paths, or compatibility branches. They are evidence of inclusion, not universal behavior.

## Reproduction notes

The original release artifacts were inspected offline from a downloaded release ZIP. A reproducible follow-up should record:

1. release URL and version;
2. SHA-256 for every artifact;
3. `file`, `readelf`, and string inventory output;
4. REA evidence IDs and target hash;
5. separate static and runtime results;
6. device model, Android version, root framework, ABI, and module version for any runtime test.

## Safety and authorization

Use analysis only on software and devices you own or are authorized to inspect. This document deliberately omits license bypass, key extraction, DRM circumvention, attestation bypass, and binary-patching instructions.
