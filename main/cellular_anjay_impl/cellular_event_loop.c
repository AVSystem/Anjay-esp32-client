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

#include "cellular_event_loop.h"
#include "cellular_anjay_impl/net_impl.h"
#include <avsystem/commons/avs_log.h>
#include <stdatomic.h>

#define CELLULAR_EVENT_LOOP_MAX_WAIT_TIME 100

static volatile atomic_bool event_loop_status;

int cellular_event_loop_run(anjay_t *anjay) {
    if (!atomic_compare_exchange_strong(&event_loop_status, &(bool) { false },
                                        true)) {
        avs_log(cellular_event_loop, ERROR, "Event loop is already running");
        return -1;
    }

    while (atomic_load(&event_loop_status)) {
        AVS_LIST(avs_net_socket_t *const) sockets = anjay_get_sockets(anjay);
        AVS_LIST(avs_net_socket_t *const) socket = NULL;

        int wait_ms = anjay_sched_calculate_wait_time_ms(
                anjay, CELLULAR_EVENT_LOOP_MAX_WAIT_TIME);

        if (sockets) {
            AVS_LIST_FOREACH(socket, sockets) {
                if (socket != NULL) {
                    bool data_received = false;
                    net_impl_check_modem_buffer(
                            (avs_net_socket_t *) avs_net_socket_get_system(
                                    *socket),
                            &data_received, wait_ms / AVS_LIST_SIZE(sockets));
                    if (data_received) {
                        int error = anjay_serve(anjay, *socket);
                        if (error) {
                            avs_log(cellular_event_loop, ERROR,
                                    "anjay_serve failed, error code %d", error);
                        }
                    }
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(wait_ms));
        }
        anjay_sched_run(anjay);
    }
    return 0;
}

int cellular_event_loop_interrupt(void) {
    return atomic_compare_exchange_strong(&event_loop_status, &(bool) { true },
                                          false)
                   ? 0
                   : -1;
}
