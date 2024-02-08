# Copyright 2021-2024 AVSystem <avsystem@avsystem.com>
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

add_custom_target( configure_git_version
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/default_config.h.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/default_config.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_version.cmake
)

add_dependencies(${COMPONENT_LIB} configure_git_version)

if (CONFIG_ANJAY_CLIENT_INTERFACE_BG96_MODULE)
     target_link_libraries(${COMPONENT_LIB} PRIVATE freertos_cellular_library)
endif()
