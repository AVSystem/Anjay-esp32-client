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
#include <netdb.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "cellular_setup.h"
#include "main.h"
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_socket_v_table.h>
#include <avsystem/commons/avs_utils.h>

#include "net_impl.h"

#define QIRD_COMMA_COUNT 2U
#define SOCKET_HAS_BUFFERED_DATA_EVENT_BIT (1UL << 0)
#define SOCKET_HAS_BUFFERED_DATA_VAL_BIT (1UL << 1)
#define SOCKET_HAS_BUFFERED_EVENT_TIMEOUT_MS 50U

static const avs_net_socket_v_table_t NET_SOCKET_VTABLE;

avs_error_t _avs_net_initialize_global_compat_state(void);
void _avs_net_cleanup_global_compat_state(void);
avs_error_t _avs_net_create_tcp_socket(avs_net_socket_t **socket,
                                       const void *socket_configuration);
avs_error_t _avs_net_create_udp_socket(avs_net_socket_t **socket,
                                       const void *socket_configuration);

typedef struct {
    const avs_net_socket_v_table_t *operations;
    int socktype;
    avs_time_duration_t recv_timeout;
    Socket_t cell_socket;
    EventGroupHandle_t event_group;
    avs_net_socket_state_t socket_state;
    char remote_hostname[256];
    size_t bytes_sent;
    size_t bytes_received;
} net_socket_impl_t;

static void cleanup_socket(net_socket_impl_t *sock) {
    if (sock->event_group) {
        vEventGroupDelete(sock->event_group);
    }
    avs_free(sock);
}

static avs_error_t
net_create_socket(avs_net_socket_t **socket_ptr,
                  const avs_net_socket_configuration_t *configuration,
                  int socktype) {
    avs_log(net_impl_cellular, TRACE, "In net_create_socket");

    assert(socket_ptr);
    assert(!*socket_ptr);

    (void) configuration;

    net_socket_impl_t *socket =
            (net_socket_impl_t *) avs_calloc(1, sizeof(net_socket_impl_t));
    EventGroupHandle_t event_group = xEventGroupCreate();

    if (!socket || !event_group) {
        cleanup_socket(socket);
        return avs_errno(AVS_ENOMEM);
    }

    socket->event_group = event_group;
    socket->operations = &NET_SOCKET_VTABLE;
    socket->socktype = socktype;
    socket->socket_state = AVS_NET_SOCKET_STATE_CLOSED;
    socket->recv_timeout = avs_time_duration_from_scalar(30, AVS_TIME_S);

    xEventGroupClearBits(socket->event_group,
                         SOCKET_HAS_BUFFERED_DATA_EVENT_BIT
                                 | SOCKET_HAS_BUFFERED_DATA_VAL_BIT);

    *socket_ptr = (avs_net_socket_t *) socket;

    return AVS_OK;
}

static avs_error_t
net_connect(avs_net_socket_t *sock_, const char *host, const char *port) {
    avs_log(net_impl_cellular, TRACE, "In net_connect");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    char resolved_ip[CELLULAR_IP_ADDRESS_MAX_SIZE + 1];
    int64_t timeout;

    unsigned long port_int_ul = strtoul(port, NULL, 10);
    uint16_t port_int;
    if (port_int_ul <= UINT16_MAX) {
        port_int = (uint16_t) port_int_ul;
    } else {
        return avs_errno(AVS_EOVERFLOW);
    }

    if (avs_time_duration_to_scalar(&timeout, AVS_TIME_MS,
                                    sock->recv_timeout)) {
        timeout = UINT32_MAX;
    }

    if (timeout > UINT32_MAX) {
        timeout = UINT32_MAX;
    }

    if (Cellular_GetHostByName(CellularHandle, CellularSocketPdnContextId, host,
                               resolved_ip)) {
        return avs_errno(AVS_EADDRNOTAVAIL);
    }
    avs_log(net_impl_cellular, TRACE,
            "Connecting to host: %s with IP addr: %s on port %" PRIu16
            " with timeout of %" PRId64,
            host, resolved_ip, port_int, timeout);

    if (avs_simple_snprintf(sock->remote_hostname,
                            sizeof(sock->remote_hostname), "%s", host)
            < 0) {
        return avs_errno(AVS_EIO);
    }

    if (Sockets_Connect(&sock->cell_socket, resolved_ip, port_int, 30000,
                        (uint32_t) timeout, sock->socktype)) {
        return avs_errno(AVS_ECONNREFUSED);
    }

    sock->socket_state = AVS_NET_SOCKET_STATE_CONNECTED;
    return AVS_OK;
}

static avs_error_t
net_send(avs_net_socket_t *sock_, const void *buffer, size_t buffer_length) {
    avs_log(net_impl_cellular, TRACE, "In net_send");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    int32_t written = 0;

    written = Sockets_Send(sock->cell_socket, buffer, buffer_length);
    if (written >= 0 && written == buffer_length) {
        sock->bytes_sent += (size_t) written;
        return AVS_OK;
    }

    return avs_errno(AVS_EIO);
}

static avs_error_t net_receive(avs_net_socket_t *sock_,
                               size_t *out_bytes_received,
                               void *buffer,
                               size_t buffer_length) {
    avs_log(net_impl_cellular, TRACE, "In net_receive");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    int64_t timeout;

    if (avs_time_duration_to_scalar(&timeout, AVS_TIME_MS,
                                    sock->recv_timeout)) {
        timeout = UINT32_MAX;
    }

    if (timeout > UINT32_MAX) {
        timeout = UINT32_MAX;
    }

    if (Sockets_SetupSocketRecvTimeout(sock->cell_socket,
                                       pdMS_TO_TICKS(timeout))) {
        return avs_errno(AVS_EIO);
    }

    int32_t bytes_received =
            Sockets_Recv(sock->cell_socket, buffer, buffer_length);
    if (bytes_received < 0) {
        return avs_errno(AVS_EIO);
    }
    *out_bytes_received = (size_t) bytes_received;
    sock->bytes_received += (size_t) bytes_received;
    if (buffer_length > 0 && sock->socktype == SOCK_DGRAM
            && (size_t) bytes_received == buffer_length) {
        return avs_errno(AVS_EMSGSIZE);
    }

    return AVS_OK;
}

static avs_error_t net_close(avs_net_socket_t *sock_) {
    avs_log(net_impl_cellular, TRACE, "In net_close");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    sock->socket_state = AVS_NET_SOCKET_STATE_CLOSED;
    Sockets_Disconnect(sock->cell_socket);
    sock->cell_socket = NULL;
    return AVS_OK;
}

static avs_error_t net_cleanup(avs_net_socket_t **sock_ptr) {
    avs_log(net_impl_cellular, TRACE, "In net_cleanup");

    avs_error_t err = AVS_OK;
    if (sock_ptr && *sock_ptr) {
        net_socket_impl_t *sock = (net_socket_impl_t *) *sock_ptr;
        err = net_close((avs_net_socket_t *) sock);
        cleanup_socket(sock);
        *sock_ptr = NULL;
    }
    return err;
}

static const void *net_system_socket(avs_net_socket_t *sock_) {
    return sock_;
}

static avs_error_t net_get_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t *out_option_value) {
    avs_log(net_impl_cellular, TRACE, "In net_get_opt");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;

    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        out_option_value->recv_timeout = sock->recv_timeout;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_STATE:
        out_option_value->state = sock->socket_state;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_INNER_MTU:
        out_option_value->mtu =
                AVS_MIN(CELLULAR_MAX_SEND_DATA_LEN, CELLULAR_MAX_RECV_DATA_LEN);
        return AVS_OK;
    case AVS_NET_SOCKET_HAS_BUFFERED_DATA:
        net_impl_check_modem_buffer(sock_, &out_option_value->flag,
                                    SOCKET_HAS_BUFFERED_EVENT_TIMEOUT_MS);
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_BYTES_SENT:
        out_option_value->bytes_sent = sock->bytes_sent;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_BYTES_RECEIVED:
        out_option_value->bytes_received = sock->bytes_received;
        return AVS_OK;
    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

static avs_error_t net_set_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t option_value) {
    avs_log(net_impl_cellular, TRACE, "In net_set_opt");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;

    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        sock->recv_timeout = option_value.recv_timeout;
        return AVS_OK;

    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

static avs_error_t net_remote_host(avs_net_socket_t *sock_,
                                   char *out_buffer,
                                   size_t out_buffer_size) {
    avs_log(net_impl_cellular, TRACE, "In net_remote_host");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    if (avs_simple_snprintf(out_buffer, out_buffer_size, "%s",
                            sock->cell_socket->cellularSocketHandle
                                    ->remoteSocketAddress.ipAddress.ipAddress)
            < 0) {
        return avs_errno(AVS_EIO);
    }
    return AVS_OK;
}

static avs_error_t net_remote_port(avs_net_socket_t *sock_,
                                   char *out_buffer,
                                   size_t out_buffer_size) {
    avs_log(net_impl_cellular, TRACE, "In net_remote_port");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    if (avs_simple_snprintf(out_buffer, out_buffer_size, "%" PRIu16,
                            sock->cell_socket->cellularSocketHandle
                                    ->remoteSocketAddress.port)
            < 0) {
        return avs_errno(AVS_EIO);
    }
    return AVS_OK;
}

static avs_error_t net_remote_hostname(avs_net_socket_t *sock_,
                                       char *out_buffer,
                                       size_t out_buffer_size) {
    avs_log(net_impl_cellular, TRACE, "In net_remote_hostname");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    if (avs_simple_snprintf(out_buffer, out_buffer_size, "%s",
                            sock->remote_hostname)
            < 0) {
        return avs_errno(AVS_EIO);
    }
    return AVS_OK;
}

static const avs_net_socket_v_table_t NET_SOCKET_VTABLE = {
    .connect = net_connect,
    .send = net_send,
    .receive = net_receive,
    .close = net_close,
    .cleanup = net_cleanup,
    .get_system_socket = net_system_socket,
    .get_remote_host = net_remote_host,
    .get_remote_hostname = net_remote_hostname,
    .get_remote_port = net_remote_port,
    .get_opt = net_get_opt,
    .set_opt = net_set_opt,
};

avs_error_t _avs_net_create_udp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr, (const avs_net_socket_configuration_t *) configuration,
            SOCK_DGRAM);
}

avs_error_t _avs_net_create_tcp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr, (const avs_net_socket_configuration_t *) configuration,
            SOCK_STREAM);
}

avs_error_t _avs_net_initialize_global_compat_state(void) {
    return AVS_OK;
}

void _avs_net_cleanup_global_compat_state(void) {}

static CellularPktStatus_t
net_impl_check_modem_buffer_callback(CellularHandle_t cellularHandle,
                                     const CellularATCommandResponse_t *pAtResp,
                                     void *pData,
                                     uint16_t dataLen) {
    avs_log(net_impl_cellular, TRACE,
            "In net_impl_check_modem_buffer_callback");

    net_socket_impl_t *sock = (net_socket_impl_t *) pData;

    CellularATCommandLine_t *itm = pAtResp->pItm;
    if (pAtResp->status) { // modem returns success
        char *save_ptr;
        char *unread_count_str = NULL;
        unread_count_str = avs_strtok(itm->pLine, ",", &save_ptr);
        for (uint8_t i = 0; i < QIRD_COMMA_COUNT && unread_count_str; i++) {
            unread_count_str = avs_strtok(NULL, ",", &save_ptr);
            if (!unread_count_str) {
                return 0;
            }
        }
        if (strcmp(unread_count_str, "0")) {
            xEventGroupSetBits(sock->event_group,
                               SOCKET_HAS_BUFFERED_DATA_VAL_BIT);
        } else {
            xEventGroupClearBits(sock->event_group,
                                 SOCKET_HAS_BUFFERED_DATA_VAL_BIT);
        }
        xEventGroupSetBits(sock->event_group,
                           SOCKET_HAS_BUFFERED_DATA_EVENT_BIT);
    }
    return 0;
}

/* Is there any unread data in modem buffer? */
avs_error_t net_impl_check_modem_buffer(avs_net_socket_t *sock_,
                                        bool *buffer_status,
                                        uint32_t timeout_milliseconds) {
    avs_log(net_impl_cellular, TRACE, "In net_impl_check_modem_buffer");

    net_socket_impl_t *sock = (net_socket_impl_t *) sock_;
    EventBits_t event_bits;
    int64_t start_timestamp = 0;
    int64_t current_timestamp = 0;
    int64_t time_elapsed = 0;
    char at_command[CELLULAR_AT_CMD_MAX_SIZE];

    xEventGroupClearBits(sock->event_group,
                         SOCKET_HAS_BUFFERED_DATA_EVENT_BIT
                                 | SOCKET_HAS_BUFFERED_DATA_VAL_BIT);

    if (avs_time_real_to_scalar(&start_timestamp, AVS_TIME_MS,
                                avs_time_real_now())) {
        return avs_errno(AVS_EIO);
    }

    avs_simple_snprintf(at_command, CELLULAR_AT_CMD_MAX_SIZE,
                        "AT+QIRD=%" PRIu32 ",0",
                        sock->cell_socket->cellularSocketHandle->socketId);
    for (;;) {
        if (Cellular_ATCommandRaw(CellularHandle, "+QIRD", at_command,
                                  CELLULAR_AT_MULTI_DATA_WO_PREFIX,
                                  &net_impl_check_modem_buffer_callback, sock,
                                  sizeof(net_socket_impl_t))) {
            return avs_errno(AVS_EIO);
        }

        event_bits = xEventGroupWaitBits(sock->event_group,
                                         SOCKET_HAS_BUFFERED_DATA_EVENT_BIT,
                                         pdTRUE,
                                         pdTRUE,
                                         pdMS_TO_TICKS(timeout_milliseconds
                                                       - time_elapsed));

        if (event_bits & SOCKET_HAS_BUFFERED_DATA_EVENT_BIT) {
            *buffer_status = !!(event_bits & SOCKET_HAS_BUFFERED_DATA_VAL_BIT);
        } else {
            return avs_errno(AVS_ETIMEDOUT);
        }

        if (*buffer_status) {
            return AVS_OK;
        }

        if (avs_time_real_to_scalar(&current_timestamp, AVS_TIME_MS,
                                    avs_time_real_now())) {
            return avs_errno(AVS_EIO);
        }

        time_elapsed = current_timestamp - start_timestamp;

        if (timeout_milliseconds < time_elapsed) {
            return avs_errno(AVS_ETIMEDOUT);
        }
    }
}
