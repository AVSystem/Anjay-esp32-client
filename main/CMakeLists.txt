# Copyright 2021 AVSystem <avsystem@avsystem.com>
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

idf_component_register(SRCS "main.c"
                            "utils.c"
                            "objects/device.c"
                            "objects/light_control.c"
                            "objects/push_button.c"
                       INCLUDE_DIRS ".")

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

target_link_libraries(${COMPONENT_LIB} PRIVATE anjay)
