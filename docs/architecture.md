# Firmware architecture

## Startup path

`main/main.cc` creates the application singleton. The board factory selects `CustomBoard`, registered from `main/boards/esp32s3-chat-badge/esp32s3_chat_badge.cc` under the legacy identifier `mz46-s3-dz`.

The board constructor initializes, in order:

1. battery and charging monitor;
2. idle/restart timer;
3. CST816D touch I²C bus;
4. QSPI display bus and JD9855 panel;
5. LVGL product UI and CST816D touch input;
6. microSD and USB-presence detection;
7. material indexing and chat-background service;
8. button callbacks and MCP tools;
9. VB6824 audio and offline voice-command callbacks.

## Major modules

| Path | Responsibility |
|---|---|
| `main/application.*` | Device state machine, connection and audio orchestration |
| `main/protocols/` | WebSocket and MQTT transport |
| `main/audio/` | Codec abstraction, audio service and optional processors |
| `main/display/` | LVGL UI, pages, resources, material loading and page manager |
| `main/services/` | Alarm and product-level services |
| `main/boards/common/` | Shared Wi-Fi, button, power and board abstractions |
| `main/boards/esp32s3-chat-badge/` | Product pin map and peripheral initialization |
| `components/vb6824/` | VB6824 UART protocol and prebuilt support library |
| `components/doit_min_program_upload/` | microSD material transfer, indexing, decoding and playback |

## Audio path

For this board, `VbAduioCodec` is selected. The VB6824 sends 40-byte encoded input frames to the ESP32-S3 and accepts PCM output. Hardware wake-word and voice-command notifications are delivered through a UART callback. ESP-SR AFE and software wake-word processing are disabled in the board defaults.

## Storage and USB ownership

At normal runtime, ESP-IDF owns the microSD card and mounts it at `/sdcard`. LVGL paths are mapped to directories such as `/sdcard/photo` and `/sdcard/animation`. When USB MSC mode is requested, file access is blocked, the VFS is unmounted, and TinyUSB takes ownership. On exit or USB removal, the firmware remounts the card and rescans materials.

## Network and OTA

The default provisioning mode is SoftAP. Runtime protocol information is obtained from the normal OTA service unless fixed WebSocket mode is selected. A legacy custom OTA endpoint is also enabled by default for compatibility; production forks should replace it with a controlled HTTPS service.

## Compatibility identifiers

The CMake project remains `xiaozhi`, firmware version remains controlled by `PROJECT_VER`, and the board type/name remain `mz46-s3-dz`. Changing any of these may affect OTA metadata, backend device classification, or existing manufacturing tools.
