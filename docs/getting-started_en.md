# ESP32-S3 Chat Badge getting started

[中文](getting-started.md) · [Back to project home](../README_en.md)

This guide is for developers building and bringing up **MZ46-S3-DZ V1.0** for the first time. Run all commands from the repository root.

## 1. Understand the delivery boundary

The repository provides:

- ESP32-S3 device firmware source;
- Board support, UI, audio, protocols, OTA, and microSD/USB MSC features;
- ESP-IDF build scripts and GitHub Actions;
- Hardware, protocol, and release documentation.

The repository does not provide:

- A standalone deployable AI cloud service;
- Production accounts, tokens, device keys, or signing keys;
- The complete commercial microSD material package;
- Source for the internal VB6824 prebuilt libraries or the complete raw VB6824 OTA protocol.

A successful firmware build and a working AI conversation are therefore separate acceptance items.

## 2. Prepare the hardware

- An MZ46-S3-DZ V1.0 board;
- A reliable USB data cable and an adequately rated USB supply;
- A microSD card when assets or USB MSC are required;
- A 2.4 GHz Wi-Fi network;
- Permission to access the serial/USB device.

See [`hardware/README.md`](hardware/README.md) for pins and the schematic.

## 3. Install the development environment

ESP-IDF `v5.4.4` is recommended. The source permits 5.4.x–5.5.x, but using the same version as CI reduces dependency and VB6824 ABI differences.

### Linux / macOS

After installing ESP-IDF through Espressif's normal installation flow, initialize each new terminal:

```bash
source ~/esp/esp-idf/export.sh
idf.py --version
```

The reported version should include `v5.4.4`.

### Windows

Install ESP-IDF `v5.4.4` and open an **ESP-IDF PowerShell**. Do not build from a normal PowerShell session that has not initialized the ESP-IDF environment.

### Docker

With Docker Desktop or Docker Engine installed, a local ESP-IDF installation is optional:

```bash
./tools/build_docker.sh
```

## 4. Get the source

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge
git submodule status
```

The project uses ESP-IDF Component Manager and does not require additional Git submodules. No output from the final command is normal.

## 5. Run static checks

```bash
python3 tools/check_repo.py
```

The checker verifies:

- Required board files;
- Resolvable CMake references;
- Alignment and overlap of the 16 MB partition table;
- Absence of internal paths, common token formats, private keys, and plaintext password logs;
- Absence of unrelated boards and obsolete components.

## 6. Build

Linux / macOS:

```bash
./tools/build.sh
```

Windows:

```powershell
.\tools\build.ps1
```

The first build downloads managed components from the ESP Component Registry. Configure an appropriate proxy or mirror in a restricted network; do not commit `managed_components/` merely to work around a local network issue.

A successful build should create at least:

```text
build/xiaozhi.bin
build/bootloader/bootloader.bin
build/partition_table/partition-table.bin
build/flasher_args.json
```

Create a full flash image with:

```bash
./tools/merge_bin.sh
```

## 7. Flash

Identify the port first:

```bash
# Linux
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# macOS
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null
```

Flash and open the monitor:

```bash
./tools/flash.sh /dev/ttyACM0
```

macOS example:

```bash
./tools/flash.sh /dev/cu.usbmodemXXXX
```

Windows example:

```powershell
idf.py -B build -p COM5 flash monitor
```

For a first flash or after a partition-table change, erase the full chip first:

```bash
idf.py -B build -p /dev/ttyACM0 erase-flash
./tools/flash.sh /dev/ttyACM0
```

A full erase removes Wi-Fi credentials, NVS settings, and OTA state.

## 8. Provision Wi-Fi for the first time

When no credentials are stored, the device enters SoftAP provisioning:

1. Read the SSID beginning with `MZ` from the display;
2. Connect a phone or computer to that access point;
3. Open `http://192.168.4.1`;
4. Select a 2.4 GHz Wi-Fi network, enter the password, and save;
5. Watch the serial log for an IP address and the subsequent OTA/configuration request.

If the captive portal does not open automatically, enter the URL manually. If the access point is absent, first verify power, the display, boot logs, and the selected provisioning mode.

## 9. Configure the AI service

### Dynamic configuration (default)

The firmware calls `CONFIG_OTA_URL`. Its response may provide:

- `mqtt`: MQTT connection settings and publish topic;
- `websocket`: WebSocket URL, token, and protocol version;
- `server_time`: server time;
- `firmware`: an optional update version and download URL.

MQTT is selected when present. WebSocket is selected when MQTT is absent and WebSocket configuration is present.

### Fixed WebSocket

```bash
idf.py -B build menuconfig
```

Enable:

```text
Xiaozhi Assistant
└── Force fixed WebSocket connection
```

Enter the WebSocket URL and access token, then rebuild and flash. When the token contains no authentication scheme, firmware automatically prefixes it with `Bearer `.

### Minimum backend compatibility

- Reply to the client `hello` within 10 seconds;
- Return the correct `transport`, `session_id`, and `audio_params`;
- Receive device Opus audio and send supported control messages and audio;
- Handle `listen`, `abort`, `mcp`, and session close correctly;
- Prefer an output sample rate compatible with the 16 kHz PCM playback path to avoid unnecessary resampling artifacts.

See [`protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md`](protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md) for all fields.

## 10. Prepare microSD assets

The complete commercial asset package is not included. For images, animations, or mini-program content:

1. Format a compatible microSD card;
2. Copy files using the directory layout of the corresponding product asset package;
3. Insert the card while powered off and reboot;
4. Check the mount and material-scan logs;
5. To access the card from a computer, connect USB and long-press the BOOT/mode button to enter USB MSC.

Do not allow the device and computer to write the card simultaneously. Eject the disk safely before leaving USB mode or removing power.

## 11. Minimum acceptance checklist

- [ ] `python3 tools/check_repo.py` passes;
- [ ] A full ESP-IDF v5.4.4 build succeeds;
- [ ] Bootloader, partition table, and application flash correctly;
- [ ] Serial output reports `mz46-s3-dz/2.0.1` or the current release version;
- [ ] LCD, touch, and backlight work;
- [ ] SoftAP provisioning and 2.4 GHz Wi-Fi work;
- [ ] VB6824 input, output, volume, and wake word work;
- [ ] A WebSocket or MQTT/UDP session works;
- [ ] microSD mount, material scan, and USB MSC work;
- [ ] Battery, charging, and sleep behavior work;
- [ ] OTA update and rollback are tested on a non-production deployment.

## 12. Troubleshooting

### `idf.py: command not found`

The current shell has not loaded ESP-IDF:

```bash
source ~/esp/esp-idf/export.sh
```

### VB6824 link or ABI failure

Use ESP-IDF 5.4.x or 5.5.x, preferably 5.4.4. Do not move directly to an unvalidated 5.6+ toolchain.

### The build works but AI interaction does not

Check in order:

1. Wi-Fi is connected;
2. The OTA/configuration endpoint returns HTTP 200 and valid JSON;
3. MQTT/WebSocket settings were written to NVS;
4. Tokens, topics, certificates, and ports are correct;
5. The backend returns `hello` within 10 seconds;
6. Audio frames match the selected protocol version and codec settings.

### The display works but product assets are missing

The repository does not contain the complete commercial microSD asset package. Verify the card mount, directories, and file formats.

### USB MSC does not expose a disk

Verify that the cable carries data, a USB host is connected, and the microSD card mounted successfully before long-pressing the BOOT/mode button. Inspect USB detection, unmount, and TinyUSB logs.

## 13. Compatibility items that should not be changed casually

- CMake project name `xiaozhi`;
- Board name `mz46-s3-dz`;
- Flash partition offsets and sizes;
- WebSocket/MQTT NVS namespaces and keys;
- OTA request headers and response fields;
- VB6824 UART command values.

A change to any of these requires corresponding protocol documentation, migration logic, backend changes, manufacturing-tool updates, and an old-firmware compatibility review.
