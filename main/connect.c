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

/* Common functions for protocol examples, to establish Wi-Fi connection.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include <string.h>

#include "connect.h"

static wifi_config_t wifi_config;

#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6
#    define MAX_IP6_ADDRS_PER_NETIF (5)
#    define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (2)

#    if defined(CONFIG_ANJAY_WIFI_CONNECT_IPV6_PREF_LOCAL_LINK)
#        define ANJAY_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL
#    elif defined(CONFIG_ANJAY_WIFI_CONNECT_IPV6_PREF_GLOBAL)
#        define ANJAY_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_GLOBAL
#    elif defined(CONFIG_ANJAY_WIFI_CONNECT_IPV6_PREF_SITE_LOCAL)
#        define ANJAY_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_SITE_LOCAL
#    elif defined(CONFIG_ANJAY_WIFI_CONNECT_IPV6_PREF_UNIQUE_LOCAL)
#        define ANJAY_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_UNIQUE_LOCAL
#    endif // if-elif ANJAY_WIFI_CONNECT_IPV6_PREF_...

#else
#    define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (1)
#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6

#define MAX_WAITING_TIME_FOR_IP 15000 // in ms

static xSemaphoreHandle s_semph_get_ip_addrs;
static esp_netif_t *s_anjay_esp_netif = NULL;

#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6
static esp_ip6_addr_t s_ipv6_addr;

/* types of ipv6 addresses to be displayed on ipv6 events */
static const char *s_ipv6_addr_types[] = { "ESP_IP6_ADDR_IS_UNKNOWN",
                                           "ESP_IP6_ADDR_IS_GLOBAL",
                                           "ESP_IP6_ADDR_IS_LINK_LOCAL",
                                           "ESP_IP6_ADDR_IS_SITE_LOCAL",
                                           "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
                                           "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6" };
#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6

static const char *TAG = "anjay_connect";

static esp_netif_t *wifi_start(void);
static void wifi_stop(void);

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the
 * module TAG, so it returns true if the specified netif is owned by this module
 */
static bool is_our_netif(const char *prefix, esp_netif_t *netif) {
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

/* set up connection, Wi-Fi */
static void start(void) {

    s_anjay_esp_netif = wifi_start();

    /* create semaphore if at least one interface is active */
    s_semph_get_ip_addrs =
            xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);
}

/* tear down connection, release resources */
static void stop(void) {
    wifi_stop();
}

static esp_ip4_addr_t s_ip_addr;

static void on_got_ip(void *arg,
                      esp_event_base_t event_base,
                      int32_t event_id,
                      void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored",
                 esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
             esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xSemaphoreGive(s_semph_get_ip_addrs);
}

#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6

static void on_got_ipv6(void *arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void *event_data) {
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *) event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv6 from another netif: ignored");
        return;
    }
    esp_ip6_addr_type_t ipv6_type =
            esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGI(TAG,
             "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s",
             esp_netif_get_desc(event->esp_netif), IPV62STR(event->ip6_info.ip),
             s_ipv6_addr_types[ipv6_type]);
    if (ipv6_type == ANJAY_CONNECT_PREFERRED_IPV6_TYPE) {
        memcpy(&s_ipv6_addr, &event->ip6_info.ip, sizeof(s_ipv6_addr));
        xSemaphoreGive(s_semph_get_ip_addrs);
    }
}

#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6

esp_err_t connect_internal(wifi_config_t *conf) {
    wifi_config = *conf;
    start();
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
    ESP_LOGI(TAG, "Waiting for IP(s)");
    for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i) {
        if (xSemaphoreTake(s_semph_get_ip_addrs,
                           pdMS_TO_TICKS(MAX_WAITING_TIME_FOR_IP))
                == pdFALSE) {
            disconnect_internal();
            return ESP_FAIL;
        }
    }
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Connected to %s", esp_netif_get_desc(netif));
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));

            ESP_LOGI(TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));
#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6
            esp_ip6_addr_t ip6[MAX_IP6_ADDRS_PER_NETIF];
            int ip6_addrs = esp_netif_get_all_ip6(netif, ip6);
            for (int j = 0; j < ip6_addrs; ++j) {
                esp_ip6_addr_type_t ipv6_type =
                        esp_netif_ip6_get_addr_type(&(ip6[j]));
                ESP_LOGI(TAG, "- IPv6 address: " IPV6STR ", type: %s",
                         IPV62STR(ip6[j]), s_ipv6_addr_types[ipv6_type]);
            }
#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6
        }
    }
    return ESP_OK;
}

esp_err_t disconnect_internal(void) {
    if (s_semph_get_ip_addrs == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    vSemaphoreDelete(s_semph_get_ip_addrs);
    s_semph_get_ip_addrs = NULL;
    stop();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&stop));
    return ESP_OK;
}

static void on_wifi_disconnect(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data) {
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6

static void on_wifi_connect(void *esp_netif,
                            esp_event_base_t event_base,
                            int32_t event_id,
                            void *event_data) {
    esp_netif_create_ip6_linklocal(esp_netif);
}

#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6

static esp_netif_t *wifi_start(void) {
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config =
            ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection
    // details (IP, gw, mask)
    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               WIFI_EVENT_STA_DISCONNECTED,
                                               &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &on_got_ip, NULL));
#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_register(
            WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect, netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6,
                                               &on_got_ipv6, NULL));
#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

static void wifi_stop(void) {
    esp_netif_t *wifi_netif = s_anjay_esp_netif;
    ESP_ERROR_CHECK(esp_event_handler_unregister(
            WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                 &on_got_ip));
#ifdef CONFIG_ANJAY_WIFI_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6,
                                                 &on_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(
            WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect));
#endif // CONFIG_ANJAY_WIFI_CONNECT_IPV6
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(
            esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
    s_anjay_esp_netif = NULL;
}
