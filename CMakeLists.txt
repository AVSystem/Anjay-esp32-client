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

# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.6)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(anjay-esp32-client)

if(CONFIG_PARTITION_TABLE_CUSTOM)
    if(CONFIG_PARTITION_TABLE_CUSTOM_FILENAME STREQUAL "partitions.csv")
        spiffs_create_partition_image(storage graphics FLASH_IN_PROJECT)
    endif()
endif()
