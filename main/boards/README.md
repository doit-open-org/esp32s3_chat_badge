# Board support

This repository intentionally supports one product board:

- `esp32s3-chat-badge/` — MZ46-S3-DZ V1.0, legacy runtime identifier `mz46-s3-dz`

The application name and legacy board identifier are retained for compatibility with existing OTA metadata and backend device classification. Do not rename either value without documenting the migration path.

Board-specific files:

- `esp32s3_chat_badge.cc` — hardware initialization and product behavior
- `config.h` — pin map and peripheral constants
- `sdkconfig.defaults` — board build defaults
- `CMakeLists.txt` — board source registration
- `README.md` — detailed pin and peripheral notes

Shared code under `common/` is compiled through the explicit source list in `main/CMakeLists.txt`; files are not globbed. Add a new common translation unit there only when this board actually uses it.
