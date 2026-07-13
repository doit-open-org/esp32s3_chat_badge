# ESP32-S3 Chat Badge board integration.
# Keep the legacy board identifier for OTA/server compatibility.
set(BOARD_TYPE "mz46-s3-dz")
set(BOARD_NAME "mz46-s3-dz")

list(APPEND SOURCES
    "${BOARD_PATH}/esp32s3_chat_badge.cc"
    "${BOARD_PATH}/esp_lcd_jd9855.c"
    "${BOARD_PATH}/esp_lcd_touch_cst816d.c"
    "${BOARD_PATH}/zw_sdcard.c"
)

list(APPEND INCLUDE_DIRS "${BOARD_PATH}")
