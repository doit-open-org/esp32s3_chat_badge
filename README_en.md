# ESP32-S3 Chat Badge

[中文](README.md)

[![ESP-IDF Build](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml/badge.svg)](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml)

Open-source firmware for the Wi-Fi electronic chat badge based on **ESP32-S3, a 360 × 360 touch display, a VB6824 audio/wake-word coprocessor, and microSD storage**. It is derived from `xiaozhi-esp32`. The application name `xiaozhi` and legacy board identifier `mz46-s3-dz` are intentionally retained for OTA and server compatibility.

## Hardware

| Item | Configuration |
|---|---|
| MCU | ESP32-S3, 240 MHz |
| Flash / PSRAM | 16 MB QIO Flash / Octal PSRAM at 80 MHz |
| Display | JD9855, 360 × 360, QSPI |
| Touch | CST816D over I²C |
| Audio | VB6824 over UART |
| Storage | microSD over 4-bit SDMMC |
| USB | TinyUSB Mass Storage for the microSD card |
| Board revision | MZ46-S3-DZ V1.0 |

See [`docs/hardware/README.md`](docs/hardware/README.md) for the pin map and schematic.

## Features

- SoftAP Wi-Fi provisioning with the default `MZ` SSID prefix
- Touch UI, image/GIF decoding, chat status and emoji display
- VB6824 audio input/output and offline voice commands
- XiaoZhi protocol foundation and device-side MCP tools
- microSD material indexing and USB Mass Storage mode
- Battery, charging, backlight and idle-power management
- Dual OTA partitions and custom OTA checks
- Multiple mini-program/PSD package support

## Build

ESP-IDF **v5.4.x** is recommended. CI and Docker are pinned to **v5.4.4**; the supported range is 5.4.x–5.5.x because those are the bundled VB6824 library ABIs.

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge
source ~/esp/esp-idf/export.sh
python3 tools/check_repo.py
./tools/build.sh
```

A containerized build is also available:

```bash
./tools/build_docker.sh
```

Windows ESP-IDF PowerShell:

```powershell
python tools/check_repo.py
.\tools\build.ps1
```

Flash and monitor:

```bash
./tools/flash.sh /dev/ttyACM0
```

Create a merged image:

```bash
./tools/merge_bin.sh
```

The build defaults are loaded from the root, the ESP32-S3 target file, and `main/boards/esp32s3-chat-badge/sdkconfig.defaults`. Use `idf.py -B build menuconfig` for OTA, WebSocket, language, provisioning, and audio settings.

The legacy custom OTA endpoint is kept for compatibility and uses HTTP. Replace it with an authenticated HTTPS endpoint before production deployment. No Wi-Fi credentials or private tokens are committed.
See [`docs/network-endpoints.md`](docs/network-endpoints.md) for the complete visible endpoint inventory.

## Flash layout

The 16 MB layout matches the supplied legacy factory image: a 960 KB model/PSD partition followed by two 6 MB OTA application slots. Changing the partition table requires a full erase and makes existing OTA packages incompatible.

## Licensing

This is a mixed-license repository. Most project code is under the root MIT License, while `main/display/page_manager/` retains GPL-3.0. Third-party components, prebuilt VB6824 libraries, fonts, audio, and artwork may carry separate terms. Read [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) before distributing binaries.

## Upstream

Derived from [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32), with product-specific hardware, UI, storage, and audio integration.
