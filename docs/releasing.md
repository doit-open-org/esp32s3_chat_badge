# Release procedure

## 1. Prepare a clean environment

Use ESP-IDF v5.4.4 and start from a clean clone. Do not reuse a developer `sdkconfig`.

```bash
source ~/esp/esp-idf/export.sh
python3 tools/check_repo.py
rm -rf build managed_components dependencies.lock
./tools/build.sh
./tools/merge_bin.sh
```

Keep `dependencies.lock` as a release artifact or commit it only after reviewing that it contains no developer-local paths. Local components under `components/` must resolve as local sources.

## 2. Hardware regression

Test at least one production-equivalent MZ46-S3-DZ V1.0 board:

- clean flash and first boot;
- SoftAP provisioning and reconnection after restart;
- VB6824 wake word, recording, playback, volume and voice commands;
- display, touch, animations, photos and chat background;
- microSD boot without card, valid card, full card and corrupted filesystem cases;
- USB MSC enter, file copy, safe eject, exit and physical cable removal;
- battery percentage, charging state, backlight and automatic restart/sleep behavior;
- WebSocket/MQTT session, MCP commands and OTA from both slots.

## 3. Record provenance

Record in the release notes:

- Git commit SHA;
- `PROJECT_VER`;
- ESP-IDF version and container/toolchain digest where available;
- hardware revision;
- merged image size and SHA-256;
- partition-table checksum;
- whether the image was reproduced by GitHub Actions.

## 4. Tag and publish

```bash
git status --short
git tag -s v2.0.1 -m "ESP32-S3 Chat Badge v2.0.1"
git push origin main --tags
```

Attach these files to the GitHub Release:

- `merged-binary.bin` and its `.sha256` file;
- individual bootloader, partition-table and application binaries;
- `flasher_args.json` or `flash_args`;
- release notes and known limitations.

Do not commit firmware binaries to the source branch.

## 5. Licensing review

Before publishing a binary, verify redistribution rights for product logos, photos, audio prompts, generated fonts, mini-program assets, the GPL-3.0 page manager, and the VB6824 precompiled library. Include source and notices required by the applicable licenses.
