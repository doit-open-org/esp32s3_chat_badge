# ESP32-S3 Chat Badge board support

This directory contains all product-specific code for MZ46-S3-DZ V1.0.

- `esp32s3_chat_badge.cc`: board initialization and product behavior
- `config.h`: pin map and hardware constants
- `esp_lcd_jd9855.*`: QSPI LCD panel driver
- `esp_lcd_touch_cst816d.*`: touch driver
- `zw_sdcard.*`: SDMMC mount, guarded file access, USB MSC switching and USB detection
- `power_manager.h`: battery/charging monitor
- `sdkconfig.defaults`: product Kconfig defaults
- `CMakeLists.txt`: board source registration and legacy board identifier

The public directory name is `esp32s3-chat-badge`; the compiled board type remains `mz46-s3-dz` for compatibility.
