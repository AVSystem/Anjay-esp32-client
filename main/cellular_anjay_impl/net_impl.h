/*
 * Copyright 2021-2022 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef NET_IMPL_H
#define NET_IMPL_H

#include <avsystem/commons/avs_errno.h>
#include <avsystem/commons/avs_socket.h>

#include "cellular_common.h"
#include "sockets_wrapper.h"

extern CellularHandle_t CellularHandle;
extern uint8_t CellularSocketPdnContextId;

/**
 * Check for new data in the BG96 buffer for corresponding socket.
 *
 * @param sock_ Socket handle to operate on.
 * @param buffer_status The output parameter, return true if there is new data
 * in modem buffer. If buffer_status value is set to true then return from
 * function.
 * @param timeout_milliseconds Defines how long function should poll BG96.
 *
 * @returns
 */
avs_error_t net_impl_check_modem_buffer(avs_net_socket_t *sock_,
                                        bool *buffer_status,
                                        uint32_t timeout_milliseconds);

#endif // NET_IMPL_H
