# Changelog

All notable changes to the public repository are documented here.

## [Unreleased]

- Initial public-repository preparation.
- Relocated MZ46-S3-DZ board code to `main/boards/esp32s3-chat-badge/`.
- Removed internal Git metadata, developer caches, build products, internal paths, unrelated board implementations, and unused large source assets.
- Added reproducible ESP-IDF build scripts, static repository checks, CI workflow, hardware documentation, licensing notices, and release guidance.
- Included the pending USB MSC guard/delay fixes found in the supplied working tree.
- Removed serial/UI logging of decoded Wi-Fi passwords from SoftAP/SmartConfig, BLUFI, and acoustic provisioning paths.
- Removed the obsolete duplicate BLUFI implementation and routed factory Wi-Fi reset through `SsidManager`.
- Merged product language override strings during header generation and pinned CI to ESP-IDF v5.4.4.

## [2.0.1] - 2026-01-13

Historical factory image supplied for reference. The image reports project `xiaozhi`, board `mz46-s3-dz`, and ESP-IDF `v5.5.1-dirty`. It was not rebuilt from this cleaned public-repository commit and must be published as a separate historical release asset.
