# Define source files
set(SOURCES "audio/audio_codec.cc"
            "audio/audio_service.cc"
            "audio/codecs/vb6824_audio_codec.cc"
            "audio/processors/audio_debugger.cc"
            "display/display.cc"
            "display/lcd_display.cc"
            "display/lvgl_display/lvgl_display.cc"
            "display/lvgl_display/emoji_collection.cc"
            "display/lvgl_display/lvgl_theme.cc"
            "display/lvgl_display/lvgl_font.cc"
            "display/lvgl_display/lvgl_image.cc"
            "display/lvgl_display/gif/lvgl_gif.cc"
            "display/lvgl_display/gif/gifdec.c"
            "display/lvgl_display/jpg/image_to_jpeg.cpp"
            "display/lvgl_display/jpg/jpeg_encoder.cpp"
            "protocols/protocol.cc"
            "protocols/mqtt_protocol.cc"
            "protocols/websocket_protocol.cc"
            "mcp_server.cc"
            "system_info.cc"
            "application.cc"
            "ota.cc"
            "custom_ota.cc"
            "settings.cc"
            "device_state_event.cc"
            "assets.cc"
            "main.cc"
            )

set(INCLUDE_DIRS "." "display" "display/lvgl_display" "display/lvgl_display/jpg" "audio" "protocols" "display/page_manager/inc" "display/page"   "display/assert"   "services" "services/alarm")

# Display 相关源文件
file(GLOB DISPLAY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/display/*.c ${CMAKE_CURRENT_SOURCE_DIR}/display/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*.c ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*/*.c ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*/*/*.c ${CMAKE_CURRENT_SOURCE_DIR}/display/*/*/*/*.cc)

# Services 相关源文件
file(GLOB SERVICES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/services/*/*.c ${CMAKE_CURRENT_SOURCE_DIR}/services/*/*.cc)

# 排除音乐相关文件和 alarm_setting
list(FILTER DISPLAY_SOURCES EXCLUDE REGEX ".*/music/.*")
list(FILTER DISPLAY_SOURCES EXCLUDE REGEX ".*/alarm_setting/.*")

list(APPEND SOURCES ${DISPLAY_SOURCES})
list(APPEND SOURCES ${SERVICES_SOURCES})

# Only compile the common modules used by this single-board firmware.
# Avoid pulling cellular/camera/PMIC helpers from the multi-board upstream tree.
set(BOARD_COMMON_SOURCES
    "boards/common/afsk_demod.cc"
    "boards/common/backlight.cc"
    "boards/common/board.cc"
    "boards/common/button.cc"
    "boards/common/power_save_timer.cc"
    "boards/common/wifi_board.cc"
)
list(APPEND SOURCES ${BOARD_COMMON_SOURCES})
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards/common)

# 允许添加外部代码
if(EXTERNAL_SOURCE_PATH)
    file(GLOB EXTERNAL_SOURCES
        ${EXTERNAL_SOURCE_PATH}/*.cc
        ${EXTERNAL_SOURCE_PATH}/*.c
    )
    list(APPEND SOURCES ${EXTERNAL_SOURCES})
    list(APPEND INCLUDE_DIRS ${EXTERNAL_SOURCE_PATH}/../)
endif()

idf_build_get_property(build_components BUILD_COMPONENTS)

# Set default BUILTIN_TEXT_FONT and BUILTIN_ICON_FONT
set(BUILTIN_TEXT_FONT font_puhui_14_1)
set(BUILTIN_ICON_FONT font_awesome_14_1)

if(CONFIG_BOARD_PATH)
get_filename_component(BOARD_TYPE ${CONFIG_BOARD_PATH} NAME)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH}/CMakeLists.txt)
    message(STATUS "include ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH}/CMakeLists.txt")
    set(BOARD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH})
    include(${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH}/CMakeLists.txt)
else()
    file(GLOB BOARD_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH}/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH}/*.c
    )
    list(APPEND BOARD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards/${CONFIG_BOARD_PATH})

    list(APPEND SOURCES ${BOARD_SOURCES})
    list(APPEND INCLUDE_DIRS ${BOARD_INCLUDE_DIRS})
endif()
endif()

# Select audio processor according to Kconfig
if(CONFIG_USE_AUDIO_PROCESSOR)
    list(APPEND SOURCES "audio/processors/afe_audio_processor.cc")
else()
    list(APPEND SOURCES "audio/processors/no_audio_processor.cc")
endif()
if(CONFIG_USE_AFE_WAKE_WORD)
    list(APPEND SOURCES "audio/wake_words/afe_wake_word.cc")
elseif(CONFIG_USE_ESP_WAKE_WORD)
    list(APPEND SOURCES "audio/wake_words/esp_wake_word.cc")
elseif(CONFIG_USE_CUSTOM_WAKE_WORD)
    list(APPEND SOURCES "audio/wake_words/custom_wake_word.cc")
endif()

# Select language directory according to Kconfig
if(CONFIG_LANGUAGE_ZH_CN)
    set(LANG_DIR "zh-CN")
elseif(CONFIG_LANGUAGE_ZH_TW)
    set(LANG_DIR "zh-TW")
elseif(CONFIG_LANGUAGE_EN_US)
    set(LANG_DIR "en-US")
elseif(CONFIG_LANGUAGE_JA_JP)
    set(LANG_DIR "ja-JP")
elseif(CONFIG_LANGUAGE_KO_KR)
    set(LANG_DIR "ko-KR")
elseif(CONFIG_LANGUAGE_VI_VN)
    set(LANG_DIR "vi-VN")
elseif(CONFIG_LANGUAGE_TH_TH)
    set(LANG_DIR "th-TH")
elseif(CONFIG_LANGUAGE_DE_DE)
    set(LANG_DIR "de-DE")
elseif(CONFIG_LANGUAGE_FR_FR)
    set(LANG_DIR "fr-FR")
elseif(CONFIG_LANGUAGE_ES_ES)
    set(LANG_DIR "es-ES")
elseif(CONFIG_LANGUAGE_IT_IT)
    set(LANG_DIR "it-IT")
elseif(CONFIG_LANGUAGE_RU_RU)
    set(LANG_DIR "ru-RU")
elseif(CONFIG_LANGUAGE_AR_SA)
    set(LANG_DIR "ar-SA")
elseif(CONFIG_LANGUAGE_HI_IN)
    set(LANG_DIR "hi-IN")
elseif(CONFIG_LANGUAGE_PT_PT)
    set(LANG_DIR "pt-PT")
elseif(CONFIG_LANGUAGE_PL_PL)
    set(LANG_DIR "pl-PL")
elseif(CONFIG_LANGUAGE_CS_CZ)
    set(LANG_DIR "cs-CZ")
elseif(CONFIG_LANGUAGE_FI_FI)
    set(LANG_DIR "fi-FI")
elseif(CONFIG_LANGUAGE_TR_TR)
    set(LANG_DIR "tr-TR")
elseif(CONFIG_LANGUAGE_ID_ID)
    set(LANG_DIR "id-ID")
elseif(CONFIG_LANGUAGE_UK_UA)
    set(LANG_DIR "uk-UA")
elseif(CONFIG_LANGUAGE_RO_RO)
    set(LANG_DIR "ro-RO")
endif()




# 定义生成路径
if(NOT CONFIG_ASSETS_PATH)
    set(CONFIG_ASSETS_PATH "assets")
else()
    list(APPEND INCLUDE_DIRS ${CONFIG_ASSETS_PATH}/..)
endif()

set(LANG_JSON "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_PATH}/locales/${LANG_DIR}/language.json")
set(LANG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_PATH}/lang_config.h")
file(GLOB LANG_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_PATH}/locales/${LANG_DIR}/*.ogg)
file(GLOB COMMON_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_PATH}/common/*.ogg)

if(NOT CONFIG_ASSETS_FIX_PATH STREQUAL "")
    set(FIX_LANG_JSON "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_FIX_PATH}/${LANG_DIR}/language.json")
    file(GLOB FIX_LANG_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_FIX_PATH}/${LANG_DIR}/*.ogg)
    file(GLOB FIX_COMMON_SOUNDS ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_ASSETS_FIX_PATH}/common/*.ogg)

    foreach(fix_sound ${FIX_LANG_SOUNDS})
        get_filename_component(fix_sound_name ${fix_sound} NAME)
        foreach(sound ${LANG_SOUNDS})
            get_filename_component(sound_name ${sound} NAME)
            if(fix_sound_name STREQUAL sound_name)
                list(REMOVE_ITEM LANG_SOUNDS ${sound})
                break()
            endif()
        endforeach()
    endforeach()

    foreach(fix_sound ${FIX_COMMON_SOUNDS})
        get_filename_component(fix_sound_name ${fix_sound} NAME)
        foreach(sound ${COMMON_SOUNDS})
            get_filename_component(sound_name ${sound} NAME)
            if(fix_sound_name STREQUAL sound_name)
                list(REMOVE_ITEM COMMON_SOUNDS ${sound})
                break() 
            endif()
        endforeach()
    endforeach()

    list(APPEND LANG_SOUNDS ${FIX_LANG_SOUNDS})
    list(APPEND COMMON_SOUNDS ${FIX_COMMON_SOUNDS})

    list(APPEND INCLUDE_DIRS ${CONFIG_ASSETS_FIX_PATH}/..)
endif()

# Several feature groups are collected with GLOBs; make the final source list deterministic.
list(REMOVE_DUPLICATES SOURCES)
list(REMOVE_DUPLICATES INCLUDE_DIRS)

idf_component_register(SRCS ${SOURCES}
                    EMBED_FILES ${LANG_SOUNDS} ${COMMON_SOUNDS}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    LDFRAGMENTS "link.lf"
                    WHOLE_ARCHIVE
                    )

# Use target_compile_definitions to define BOARD_TYPE, BOARD_NAME
# If BOARD_NAME is empty, use BOARD_TYPE
if(NOT BOARD_NAME)
    set(BOARD_NAME ${BOARD_TYPE})
endif()
target_compile_definitions(${COMPONENT_LIB}
                    PRIVATE BOARD_TYPE=\"${BOARD_TYPE}\" BOARD_NAME=\"${BOARD_NAME}\"
                    PRIVATE BUILTIN_TEXT_FONT=${BUILTIN_TEXT_FONT} BUILTIN_ICON_FONT=${BUILTIN_ICON_FONT}
                    )

if(NOT CONFIG_ASSETS_FIX_PATH STREQUAL "")
add_custom_command(
    OUTPUT ${LANG_HEADER}
    COMMAND ${PYTHON} ${PROJECT_DIR}/scripts/gen_lang_fix.py
            --input "${LANG_JSON}"
            --input_fix "${FIX_LANG_JSON}"
            --output "${LANG_HEADER}"
    DEPENDS
        ${LANG_JSON}
        ${FIX_LANG_JSON}
        ${PROJECT_DIR}/scripts/gen_lang_fix.py
        ${PROJECT_DIR}/scripts/gen_lang.py
    COMMENT "Generating ${LANG_DIR} language config"
)
else()
# 添加生成规则
add_custom_command(
    OUTPUT ${LANG_HEADER}
    COMMAND ${PYTHON} ${PROJECT_DIR}/scripts/gen_lang.py
            --language "${LANG_DIR}"
            --output "${LANG_HEADER}"
    DEPENDS
        ${LANG_JSON}
        ${PROJECT_DIR}/scripts/gen_lang.py
    COMMENT "Generating ${LANG_DIR} language config"
)
endif()

# Force build generation dependencies
add_custom_target(lang_header ALL
    DEPENDS ${LANG_HEADER}
)
add_dependencies(${COMPONENT_LIB} lang_header)

# Find ESP-SR component dynamically
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "espressif__esp-sr")
        set(ESP_SR_COMPONENT ${COMPONENT})
        idf_component_get_property(ESP_SR_COMPONENT_PATH ${ESP_SR_COMPONENT} COMPONENT_DIR)
        set(ESP_SR_MODEL_PATH "${ESP_SR_COMPONENT_PATH}/model")
        break()
    endif()
endforeach()

# Find xiaozhi-fonts component dynamically
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "xiaozhi-fonts")
        set(XIAOZHI_FONTS_COMPONENT ${COMPONENT})
        idf_component_get_property(XIAOZHI_FONTS_COMPONENT_PATH ${XIAOZHI_FONTS_COMPONENT} COMPONENT_DIR)
        set(XIAOZHI_FONTS_PATH "${XIAOZHI_FONTS_COMPONENT_PATH}")
        break()
    endif()
endforeach()

if(CONFIG_BOARD_TYPE_ESP_HI)
set(URL "https://github.com/espressif2022/image_player/raw/main/test_apps/test_8bit")
set(EMOJI_DIR "${CMAKE_BINARY_DIR}/emoji")
file(MAKE_DIRECTORY ${EMOJI_DIR})

# List all files to download
set(FILES_TO_DOWNLOAD "")
list(APPEND FILES_TO_DOWNLOAD "Anger_enter.aaf" "Anger_loop.aaf" "Anger_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "happy_enter.aaf" "happy_loop.aaf" "happ_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "sad_enter.aaf" "sad_loop.aaf" "sad_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "scorn_enter.aaf" "scorn_loop.aaf" "scorn_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "left_enter.aaf" "left_loop.aaf" "left_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "right_enter.aaf" "right_loop.aaf" "right_return.aaf")
list(APPEND FILES_TO_DOWNLOAD "asking.aaf" "blink_once.aaf" "blink_quick.aaf")
list(APPEND FILES_TO_DOWNLOAD "connecting.aaf" "panic_enter.aaf" "panic_loop.aaf")
list(APPEND FILES_TO_DOWNLOAD "panic_return.aaf" "wake.aaf")

foreach(FILENAME IN LISTS FILES_TO_DOWNLOAD)
    set(REMOTE_FILE "${URL}/${FILENAME}")
    set(LOCAL_FILE "${EMOJI_DIR}/${FILENAME}")
    
    # Check if local file exists
    if(EXISTS ${LOCAL_FILE})
        message(STATUS "File ${FILENAME} already exists, skipping download")
    else()
        message(STATUS "Downloading ${FILENAME}")
        file(DOWNLOAD ${REMOTE_FILE} ${LOCAL_FILE}
             STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
        if(NOT STATUS_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download ${FILENAME} from ${URL}")
        endif()
    endif()
endforeach()

endif()


# Function to build default assets based on configuration
function(build_default_assets_bin)
    # Set output path for generated assets.bin
    set(GENERATED_ASSETS_BIN "${CMAKE_BINARY_DIR}/generated_assets.bin")
    
    # Prepare arguments for build script
    set(BUILD_ARGS
        "--sdkconfig" "${SDKCONFIG}"
        "--output" "${GENERATED_ASSETS_BIN}"
    )
    
    # Add builtin text font if defined
    if(BUILTIN_TEXT_FONT)
        list(APPEND BUILD_ARGS "--builtin_text_font" "${BUILTIN_TEXT_FONT}")
    endif()
    
    # Add default emoji collection if defined
    if(DEFAULT_EMOJI_COLLECTION)
        list(APPEND BUILD_ARGS "--emoji_collection" "${DEFAULT_EMOJI_COLLECTION}")
    endif()
    
    # Add default assets extra files if defined
    if(DEFAULT_ASSETS_EXTRA_FILES)
        list(APPEND BUILD_ARGS "--extra_files" "${DEFAULT_ASSETS_EXTRA_FILES}")
    endif()
    
    list(APPEND BUILD_ARGS "--esp_sr_model_path" "${ESP_SR_MODEL_PATH}")
    list(APPEND BUILD_ARGS "--xiaozhi_fonts_path" "${XIAOZHI_FONTS_PATH}")
    
    # Create custom command to build assets
    add_custom_command(
        OUTPUT ${GENERATED_ASSETS_BIN}
        COMMAND ${PYTHON} ${PROJECT_DIR}/scripts/build_default_assets.py ${BUILD_ARGS}
        DEPENDS
            ${SDKCONFIG}
            ${PROJECT_DIR}/scripts/build_default_assets.py
        COMMENT "Building default assets.bin based on configuration"
        VERBATIM
    )
    
    # Create target for generated assets
    add_custom_target(generated_default_assets ALL
        DEPENDS ${GENERATED_ASSETS_BIN}
    )
    
    # Set the generated file path in parent scope
    set(GENERATED_ASSETS_LOCAL_FILE ${GENERATED_ASSETS_BIN} PARENT_SCOPE)
    
    message(STATUS "Default assets build configured: ${GENERATED_ASSETS_BIN}")
endfunction()


partition_table_get_partition_info(size "--partition-name assets" "size")
partition_table_get_partition_info(offset "--partition-name assets" "offset")
if ("${size}" AND "${offset}")
    # Flash assets based on configuration
    if(CONFIG_FLASH_DEFAULT_ASSETS)
        # Build default assets based on configuration
        build_default_assets_bin()
        esptool_py_flash_to_partition(flash "assets" "${GENERATED_ASSETS_LOCAL_FILE}")
        message(STATUS "Generated default assets flash configured: ${GENERATED_ASSETS_LOCAL_FILE} -> assets partition")
    elseif(CONFIG_FLASH_CUSTOM_ASSETS)
        # Flash custom assets
        get_assets_local_file("${CONFIG_CUSTOM_ASSETS_FILE}" ASSETS_LOCAL_FILE)
        esptool_py_flash_to_partition(flash "assets" "${ASSETS_LOCAL_FILE}")
        message(STATUS "Custom assets flash configured: ${ASSETS_LOCAL_FILE} -> assets partition")
    elseif(CONFIG_FLASH_NONE_ASSETS)
        message(STATUS "Assets flashing disabled (FLASH_NONE_ASSETS)")
    endif()
else()
    message(STATUS "Assets partition not found, using v1 partition table")
endif()
