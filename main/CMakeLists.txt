# Copyright 2021-2022 AVSystem <avsystem@avsystem.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(sources
     "main.c"
     "connect.c"
     "utils.c"
     "objects/device.c"
     "objects/light_control.c"
     "objects/push_button.c"
     "objects/mpu6886.c"
     "objects/sensors.c"
     "st7789.c"
     "fontx.c"
     "lcd.c"
     "axp192.c"
     "i2c_wrapper.c"
     "firmware_update.c")

if (CONFIG_ANJAY_SECURITY_MODE_CERTIFICATES)
     set(Embedded_cert "../server_cert.der" "../client_cert.der" "../client_key.der")
else()
     set(Embedded_cert "")
endif()

if (CONFIG_ANJAY_CLIENT_INTERFACE_BG96_MODULE)
     list(APPEND sources
          "cellular_anjay_impl/net_impl.c"
          "cellular_anjay_impl/cellular_event_loop.c")
endif()

if (CONFIG_ANJAY_CLIENT_INTERFACE_ONBOARD_WIFI)
     list(APPEND sources
          "objects/wlan.c")
endif()

idf_component_register(SRCS ${sources}
                    INCLUDE_DIRS "."
                    EMBED_FILES ${Embedded_cert})

file(GLOB_RECURSE ANJAY_SOURCES
     "anjay/src/*.c"
     "anjay/deps/avs_coap/src/*.c"
     "anjay/deps/avs_commons/src/*.c")

add_library(anjay MODULE ${ANJAY_SOURCES})
target_include_directories(anjay PRIVATE
                           "anjay/src"
                           "anjay/deps/avs_coap/src"
                           "anjay/deps/avs_commons/src")
target_include_directories(anjay PUBLIC
                           "anjay-config"
                           "anjay/include_public"
                           "anjay/deps/avs_coap/include_public"
                           "anjay/deps/avs_commons/include_public")
target_link_libraries(anjay PRIVATE idf::mbedtls)

# NOTE: avs_coap contains some assertions that check if allocated memory
# follows alignment requirements. malloc() on ESP-IDF aligns everything
# to 4 bytes, even though alignof(max_align_t) == alignof(int64_t)
# == alignof(long double) == 8. This is just GCC being overly cautious,
# the maximum hardware-imposed alignment on ESP32 is actually 4 bytes.
# We disable assertions within anjay to work around that.
target_compile_definitions(anjay PRIVATE NDEBUG)

if (CONFIG_ANJAY_CLIENT_INTERFACE_BG96_MODULE)
     file(GLOB_RECURSE FREERTOS_CELLULAR_LIBRARY_SOURCES
                              "FreeRTOS-Cellular-Interface/source/*.c"
                              "FreeRTOS-Cellular-Interface/modules/bg96/*.c"
                              "FreeRTOS-Cellular-Interface/modules/cellular_platform.c")

     add_library(freertos_cellular_library MODULE ${FREERTOS_CELLULAR_LIBRARY_SOURCES})
     target_include_directories(freertos_cellular_library PRIVATE
                              "FreeRTOS-Cellular-Interface/source/include/private"
                              "FreeRTOS-Cellular-Interface/modules/bg96")
     target_include_directories(freertos_cellular_library PUBLIC
                              "FreeRTOS-Cellular-Interface/source/include"
                              "FreeRTOS-Cellular-Interface/source/include/common"
                              "FreeRTOS-Cellular-Interface/source/logging"
                              "FreeRTOS-Cellular-Interface/source/cellular_network_transport"
                              "FreeRTOS-Cellular-Interface/source/interface"
                              "FreeRTOS-Cellular-Interface/modules")
     target_link_libraries(freertos_cellular_library PRIVATE anjay)
endif()

add_custom_target( configure_git_version
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/default_config.h.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/default_config.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_version.cmake
)

add_dependencies(${COMPONENT_LIB} configure_git_version)

target_link_libraries(${COMPONENT_LIB} PRIVATE anjay)
if (CONFIG_ANJAY_CLIENT_INTERFACE_BG96_MODULE)
     target_link_libraries(${COMPONENT_LIB} PRIVATE freertos_cellular_library)
endif()
