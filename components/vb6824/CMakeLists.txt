set(srcs vb6824.c)
set(incs .)
set(assets)

if(CONFIG_VB6824_OTA_SUPPORT)
    list(APPEND assets assets/update.html)
    list(APPEND srcs https.c)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${incs}
    EMBED_TXTFILES ${assets}
    REQUIRES driver esp_wifi esp_http_client esp_http_server json esp_timer
)

if(NOT CONFIG_IDF_TARGET_ESP32S3)
    message(FATAL_ERROR "The bundled VB6824 libraries in this repository support ESP32-S3 only")
endif()

if(IDF_VERSION_MAJOR EQUAL 5 AND IDF_VERSION_MINOR EQUAL 4)
    set(lib_version "idf5.4")
elseif(IDF_VERSION_MAJOR EQUAL 5 AND IDF_VERSION_MINOR EQUAL 5)
    set(lib_version "idf5.5")
else()
    message(FATAL_ERROR
        "Bundled VB6824 libraries support ESP-IDF 5.4.x and 5.5.x only; "
        "current version is ${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")
endif()

set(vb6824_library "${CMAKE_CURRENT_SOURCE_DIR}/libs/${lib_version}/libesp32s3.a")
if(NOT EXISTS "${vb6824_library}")
    message(FATAL_ERROR "VB6824 library not found: ${vb6824_library}")
endif()

add_prebuilt_library(vbota "${vb6824_library}")
target_link_libraries(${COMPONENT_LIB} PRIVATE vbota)
