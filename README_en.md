# ESP32-S3 Chat Badge

[中文](README.md)

[![ESP-IDF Build](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml/badge.svg)](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml)

Open-source firmware for the **MZ46-S3-DZ V1.0** Wi-Fi electronic badge, built around an **ESP32-S3, a 360 × 360 round touch display, a VB6824 audio/offline wake-word coprocessor, and microSD storage**. The project is derived from `xiaozhi-esp32`. The application name `xiaozhi` and board identifier `mz46-s3-dz` are intentionally retained for OTA metadata, backend device classification, and manufacturing-tool compatibility.

> [!IMPORTANT]
> This repository contains buildable device firmware, but it does **not** include an AI backend implementation, production credentials, or the complete commercial microSD material package. After flashing, voice interaction still requires a compatible OTA/protocol service, and the full product UI may require additional assets on the microSD card.

## What is included

- SoftAP Wi-Fi provisioning with the default `MZ` SSID prefix
- 360 × 360 JD9855 QSPI LCD and CST816D capacitive touch
- VB6824 audio input, PCM output, hardware wake word, and offline voice commands
- AI sessions over WebSocket or MQTT + UDP with Opus audio
- Device-side MCP tools over JSON-RPC 2.0
- microSD material indexing, image/animation decoding, and USB Mass Storage
- Battery, charging, backlight, and idle-power management
- Dual OTA application partitions, service discovery, and custom OTA checks
- Multiple mini-program/PSD package support

## Hardware baseline

| Item | Configuration |
|---|---|
| MCU | ESP32-S3, up to 240 MHz |
| Flash / PSRAM | 16 MB QIO Flash / Octal PSRAM at 80 MHz |
| Display | JD9855, 360 × 360, QSPI |
| Touch | CST816D over I²C |
| Audio / wake word | VB6824 over 2 Mbit/s UART |
| Storage | microSD over 4-bit SDMMC |
| USB | TinyUSB MSC exposing the microSD card |
| Board revision | MZ46-S3-DZ V1.0 |

See [`docs/hardware/README.md`](docs/hardware/README.md) for the pin map, hardware assumptions, and schematic.

## Before you build

Prepare the following:

1. **MZ46-S3-DZ V1.0 hardware.** Other PCB revisions must be checked against the pin map, display, audio, power, and partition configuration.
2. **ESP-IDF v5.4.4.** The component manifest supports `>=5.4.0,<5.6.0`; the repository includes prebuilt VB6824 libraries for ESP-IDF 5.4 and 5.5.
3. **Python 3.10+, Git, and serial-port access**, or Docker for a containerized build.
4. **A compatible AI backend/OTA configuration service.** The normal flow obtains MQTT or WebSocket settings from the OTA response; fixed WebSocket settings may also be selected in `menuconfig`.
5. **A microSD card and product assets** when the complete UI, images, animations, or mini-program packages are required.

For a complete first-build and bring-up procedure, read [`docs/getting-started_en.md`](docs/getting-started_en.md).

## Quick start

### 1. Clone

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge
```

### 2. Build on Linux or macOS

```bash
# Adjust this path for your ESP-IDF installation.
source ~/esp/esp-idf/export.sh

python3 tools/check_repo.py
./tools/build.sh
```

### 3. Build on Windows PowerShell

Open an ESP-IDF PowerShell first, then run:

```powershell
python tools/check_repo.py
.\tools\build.ps1
```

### 4. Build with Docker

When ESP-IDF is not installed locally:

```bash
./tools/build_docker.sh
```

Docker and GitHub Actions are pinned to ESP-IDF `v5.4.4`.

### 5. Flash and monitor

Linux:

```bash
./tools/flash.sh /dev/ttyACM0
```

macOS:

```bash
./tools/flash.sh /dev/cu.usbmodemXXXX
```

Windows:

```powershell
idf.py -B build -p COM5 flash monitor
```

Exit the serial monitor with `Ctrl+]`.

## First boot and Wi-Fi provisioning

When no Wi-Fi credentials are stored, the device enters SoftAP provisioning mode:

1. Find the SSID beginning with `MZ` on the display;
2. Connect a phone or computer to that access point;
3. Open the provisioning page shown on the display; the default URL is `http://192.168.4.1`;
4. Select a 2.4 GHz Wi-Fi network and save the credentials;
5. After networking succeeds, the firmware continues with OTA/protocol configuration and AI service connection.

Successful Wi-Fi provisioning does not by itself provide an AI service. If the device reports that no server is available, verify the backend configuration below.

## AI backend and protocol configuration

The default startup flow first calls the OTA/device-configuration endpoint and selects a transport from the response:

1. An `mqtt` object selects MQTT control messages plus AES-CTR UDP Opus audio;
2. Otherwise, a `websocket` object selects WebSocket JSON plus binary Opus audio;
3. If neither object is present, the firmware falls back to MQTT, which still requires valid MQTT settings;
4. `CONFIG_CONNECTION_FORCE_FIXED_WEBSOCKET` bypasses dynamic selection and uses the build-time WebSocket URL/token.

Open the configuration UI with:

```bash
idf.py -B build menuconfig
```

Review the OTA URL, fixed WebSocket settings, access token, language, provisioning mode, and audio options under **Xiaozhi Assistant**. Do not commit Wi-Fi passwords, production keys, or private tokens.

Protocol references:

- [English protocol specification](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md)
- [中文协议说明](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md)
- [Visible network endpoint inventory](docs/network-endpoints.md)

> [!WARNING]
> A legacy custom OTA endpoint may still use HTTP for compatibility. Public or production deployments must replace it with a controlled HTTPS service and define firmware signing, credential rotation, and server-side authorization.

## Build outputs

The default output directory is `build/`:

| File | Purpose |
|---|---|
| `build/xiaozhi.bin` | Application image |
| `build/bootloader/bootloader.bin` | Bootloader |
| `build/partition_table/partition-table.bin` | Partition table |
| `build/flasher_args.json` | Flash arguments |
| `build/merged-binary.bin` | Full flash image created by `./tools/merge_bin.sh` |
| `build/merged-binary.bin.sha256` | SHA-256 of the merged image |

Create the merged image with:

```bash
./tools/merge_bin.sh
```

## microSD and USB Mass Storage

During normal operation, the ESP32-S3 mounts the microSD card and scans image, animation, and mini-program assets. When USB MSC mode is entered, firmware-side file access is paused, the VFS is unmounted, and the card is handed to the USB host. The card is remounted and rescanned when USB mode ends, preventing simultaneous writes by the host and device.

The current product logic attempts to toggle USB MSC on a long press of the BOOT/mode button. A connected USB host and a valid microSD card are required before the disk is exposed.

The complete commercial asset package is not included in this repository. Directory layout and supported resource formats follow the current source and the corresponding microSD package.

## Flash layout

The fixed 16 MB layout is:

| Partition | Offset | Size |
|---|---:|---:|
| NVS | `0x9000` | 16 KB |
| OTA data | `0xD000` | 8 KB |
| PHY init | `0xF000` | 4 KB |
| model / PSD | `0x10000` | 960 KB |
| OTA 0 | `0x100000` | 6 MB |
| OTA 1 | `0x700000` | 6 MB |

Changing the partition table requires a full flash erase and makes existing OTA packages incompatible.

## Documentation

| Topic | English | 中文 |
|---|---|---|
| Getting started | [Getting started](docs/getting-started_en.md) | [开始使用](docs/getting-started.md) |
| Documentation index | [Documentation](docs/README.md) | [Documentation](docs/README.md) |
| Hardware and pins | [Hardware reference](docs/hardware/README.md) | [硬件参考](docs/hardware/README.md) |
| Firmware architecture | [Architecture](docs/architecture.md) | [架构说明](docs/architecture.md) |
| Communication protocol | [English](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md) | [中文](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md) |
| Network endpoints | [Endpoint inventory](docs/network-endpoints.md) | [端点清单](docs/network-endpoints.md) |
| Release process | [Release guide](docs/releasing.md) | [发布说明](docs/releasing.md) |
| Contributing | [CONTRIBUTING.md](CONTRIBUTING.md) | [CONTRIBUTING.md](CONTRIBUTING.md) |
| Security | [SECURITY.md](SECURITY.md) | [SECURITY.md](SECURITY.md) |
| Third-party notices | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |

## Repository layout

```text
components/                       Pinned or customized local components
main/                             Application, UI, audio, protocols, and services
main/boards/esp32s3-chat-badge/   Product-specific board support
main/assets/                      Shared language and prompt resources
main/assets_overrides/            Badge-specific resources
partitions/                       16 MB partition table
scripts/                          Resource-generation scripts
tools/                            Check, build, flash, and image-merge tools
docs/                             Hardware, protocol, architecture, and release docs
.github/                          CI and Issue/PR templates
```

## Compatibility and known boundaries

- The current public target is **MZ46-S3-DZ V1.0** only.
- `xiaozhi`, `mz46-s3-dz`, the partition layout, and NVS key names are compatibility interfaces; changing them may affect OTA, backend classification, and manufacturing tools.
- The public VB6824 UART frame is documented, but the internal prebuilt-library behavior and raw VB6824 OTA stream cannot be fully specified from the public source alone.
- The WebSocket/MQTT document is a compatibility specification inferred from this firmware implementation, not an independent industry standard.
- The current `hello` declares 60 ms audio frames, while VB6824 input defaults to 40-byte/20 ms Opus frames; no explicit three-frame aggregation is visible, so third-party backends must verify audio framing on real hardware.
- CI can validate the build, but display, touch, audio, microSD, USB MSC, battery, and OTA behavior still require real-hardware regression testing.

## Contributing and releases

Before submitting changes, run:

```bash
python3 tools/check_repo.py
./tools/build.sh
./tools/merge_bin.sh
```

Read [`CONTRIBUTING.md`](CONTRIBUTING.md). Before tagging a release, follow [`docs/releasing.md`](docs/releasing.md) for hardware regression, firmware checksums, and release assets.

## Licensing and upstream

This is a mixed-license repository:

- Most project and upstream `xiaozhi-esp32` code is distributed under the root [`LICENSE`](LICENSE) (MIT);
- `main/display/page_manager/` retains GPL-3.0;
- ESP-IDF components, prebuilt VB6824 libraries, fonts, audio, images, and other assets may have separate terms.

Read [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) before distributing binaries. This project is derived from [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) and uses Espressif ESP-IDF, LVGL, and components from the ESP Component Registry.

## Contact

For technical discussion, hardware customization, or business cooperation:

- Email: [lihonggang@doit.am](mailto:lihonggang@doit.am)

WeChat:

![WeChat contact](docs/wechat_qrcode.jpg)

Discussions about ESP32, AI hardware, and embedded systems are welcome.
