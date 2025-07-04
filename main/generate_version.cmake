# Copyright 2021-2025 AVSystem <avsystem@avsystem.com>
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

# Git executable is extracted from parameters.
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always
                RESULT_VARIABLE GIT_DESCRIBE_RESULT
                OUTPUT_VARIABLE CLIENT_VERSION_S)
if(GIT_DESCRIBE_RESULT EQUAL 0)
    string(STRIP "${CLIENT_VERSION_S}" CLIENT_VERSION)
else()
    set(CLIENT_VERSION "25.05")
endif()
# Input and output files are extracted from parameters.
configure_file("${INPUT_FILE}" "${OUTPUT_FILE}" @ONLY)
