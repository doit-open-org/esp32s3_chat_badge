set(srcs
    file/doit_decode.c
    file/doit_file.c
    file/doit_ui.c
    file/file_download.c
    file/vpg_decode.c
    file/img_decode.c
    ble/doit_ble.c
    ble/gap.c
    ble/gatt_svc.c
    doit_nvs.c
)

set(incs
    include/file
    include/ble
    ${CMAKE_CURRENT_SOURCE_DIR}/../../main/display
)

# Materials are stored on microSD at runtime. The legacy LittleFS image was
# unused by the active 16 MB dual-OTA partition table and is intentionally not
# generated in this open-source project.
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${incs}
    REQUIRES
        driver
        esp_netif
        esp_event
        esp-tls
        esp_http_client
        nvs_flash
        espressif__esp_new_jpeg
        lvgl__lvgl
        espressif__esp_lvgl_port
        bt
        json
    PRIV_REQUIRES fatfs
)

# The product uses a 360 x 360 display; the mini-program BLE protocol reports
# this resolution to the companion application.
target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_LCD_ST77916_360X360=1)
