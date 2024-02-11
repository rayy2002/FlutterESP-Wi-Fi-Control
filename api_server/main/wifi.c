
#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>
#include <mdns.h>
#include <math.h>
#include <driver/gpio.h>
#include <sys/param.h>
#include <freertos/event_groups.h>
#include <esp_netif_ip_addr.h>
#include <lwip/lwip_napt.h>
#include "wifi.h"
#include "config.h"
#include "driver/gpio.h"

static const char *TAG = "WIFI";

static EventGroupHandle_t wifi_event_group;
const int WIFI_STA_GOT_IPV4_BIT = BIT0;
const int WIFI_STA_GOT_IPV6_BIT = BIT1;
const int WIFI_AP_STA_CONNECTED_BIT = BIT2;

static TaskHandle_t sta_status_task = NULL;
static TaskHandle_t sta_reconnect_task = NULL;

static wifi_config_t config_ap;
static wifi_config_t config_sta;


static bool ap_active = false;
static bool sta_active = false;
static int attempts = 1;

static bool sta_connected;
static wifi_ap_record_t sta_ap_info;
static wifi_sta_list_t ap_sta_list;

static esp_netif_t *esp_netif_ap;
static esp_netif_t *esp_netif_sta;

static void wifi_sta_status_task(void *ctx) {
    while (true) {
        sta_connected = esp_wifi_sta_get_ap_info(&sta_ap_info) == ESP_OK;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void wifi_sta_reconnect_task(void *ctx) {

    while (true) {
    	attempts++;
        ESP_LOGI(TAG, "Station Reconnecting: %s, attempts: %d", config_sta.sta.ssid, attempts);
        esp_wifi_connect();

        // Wait for next disconnect event
        vTaskSuspend(NULL);


    }
}

static void handle_sta_start(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WIFI_EVENT_STA_START");

    sta_active = true;
    esp_wifi_connect();
}

static void handle_sta_stop(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
    sta_active = false;
}

static void handle_sta_connected(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const wifi_event_sta_connected_t *event = (const wifi_event_sta_connected_t *) event_data;

    ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED: ssid: %.*s", event->ssid_len, event->ssid);

    sta_connected = true;

    attempts = 0;

    // Tracking status
    if (sta_status_task != NULL) vTaskResume(sta_status_task);

    // No longer attempting to reconnect
    if (sta_reconnect_task != NULL) vTaskSuspend(sta_reconnect_task);

}

static void handle_sta_disconnected(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const wifi_event_sta_disconnected_t *event = (const wifi_event_sta_disconnected_t *) event_data;
    char *reason;
    switch (event->reason) {
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_EXPIRE:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            reason = "AUTH";
            break;
        case WIFI_REASON_NO_AP_FOUND:
            reason = "NOT_FOUND";
            break;
        default:
            reason = "UNKNOWN";
    }

    ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED: ssid: %.*s, reason: %d (%s)", event->ssid_len, event->ssid, event->reason, reason);

    sta_connected = false;

    // No longer tracking status
    if (sta_status_task != NULL) vTaskSuspend(sta_status_task);

    // Attempting to reconnect
    if (sta_reconnect_task != NULL) vTaskResume(sta_reconnect_task);


    xEventGroupClearBits(wifi_event_group, WIFI_STA_GOT_IPV4_BIT);
    xEventGroupClearBits(wifi_event_group, WIFI_STA_GOT_IPV6_BIT);

}

static void handle_sta_auth_mode_change(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const wifi_event_sta_authmode_change_t *event = (const wifi_event_sta_authmode_change_t *) event_data;
    const char *old_auth_mode = wifi_auth_mode_name(event->old_mode);
    const char *new_auth_mode = wifi_auth_mode_name(event->new_mode);

    ESP_LOGI(TAG, "WIFI_EVENT_STA_AUTHMODE_CHANGE: old: %s, new: %s", old_auth_mode, new_auth_mode);
}

static void handle_ap_start(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
    ap_active = true;
}

static void handle_ap_stop(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
    ap_active = false;
}

static void handle_ap_sta_connected(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const wifi_event_ap_staconnected_t *event = (const wifi_event_ap_staconnected_t *) event_data;

    ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED: mac: " MACSTR, MAC2STR(event->mac));
    xEventGroupSetBits(wifi_event_group, WIFI_AP_STA_CONNECTED_BIT);

}

static void handle_ap_sta_disconnected(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const wifi_event_ap_stadisconnected_t *event = (const wifi_event_ap_stadisconnected_t *) event_data;

    ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED: mac: " MACSTR, MAC2STR(event->mac));
    gpio_set_level(GPIO_NUM_2,0);
    wifi_ap_sta_list();
    if (ap_sta_list.num == 0) {
        xEventGroupClearBits(wifi_event_group, WIFI_AP_STA_CONNECTED_BIT);

    }
}

static void handle_sta_got_ip(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const ip_event_got_ip_t *event = (const ip_event_got_ip_t *) event_data;


    ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: ip: " IPSTR "/%d, gw: " IPSTR,
            IP2STR(&event->ip_info.ip),
            ffs(~event->ip_info.netmask.addr) - 1,
            IP2STR(&event->ip_info.gw));

    xEventGroupSetBits(wifi_event_group, WIFI_STA_GOT_IPV4_BIT);
}

static void handle_sta_lost_ip(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "IP_EVENT_STA_LOST_IP");
    xEventGroupClearBits(wifi_event_group, WIFI_STA_GOT_IPV4_BIT);
}

static void handle_ap_sta_ip_assigned(void *esp_netif, esp_event_base_t base, int32_t event_id, void *event_data) {
    const ip_event_ap_staipassigned_t *event = (const ip_event_ap_staipassigned_t *) event_data;

    ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED: ip: " IPSTR, IP2STR(&event->ip));
    gpio_set_level(GPIO_NUM_2,1);
}

void wait_for_ip() {
	ESP_LOGI(TAG, "wait_for_ip");
    xEventGroupWaitBits(wifi_event_group, WIFI_STA_GOT_IPV4_BIT, false, false, portMAX_DELAY);
}

void wait_for_network() {
    xEventGroupWaitBits(wifi_event_group, WIFI_STA_GOT_IPV4_BIT | WIFI_AP_STA_CONNECTED_BIT, false, false, portMAX_DELAY);
}

void net_init() {
    esp_netif_init();

    // SoftAP
	esp_netif_ap = esp_netif_create_default_wifi_ap();

    // STA
	esp_netif_sta = esp_netif_create_default_wifi_sta();

}

void wifi_init() {

	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2,0);
	net_init();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    wifi_event_group = xEventGroupCreate();


    // Listen for WiFi and IP events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &handle_sta_start, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &handle_sta_stop, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handle_sta_connected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handle_sta_disconnected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_AUTHMODE_CHANGE, &handle_sta_auth_mode_change, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &handle_ap_start, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &handle_ap_stop, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &handle_ap_sta_connected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &handle_ap_sta_disconnected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &handle_sta_lost_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &handle_ap_sta_ip_assigned, NULL));



    // Configure and connect
    wifi_mode_t wifi_mode;
    wifi_mode = WIFI_MODE_APSTA;

    ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));

    esp_netif_ip_info_t ip_info_ap;
    esp_netif_get_ip_info(esp_netif_ap, &ip_info_ap);

    config_ap.ap.max_connection = 1;
	snprintf((char *) config_ap.ap.ssid, sizeof(config_ap.ap.ssid), "ESP__API_Server");
	config_ap.ap.ssid_len = strlen((char *) config_ap.ap.ssid);
	config_set_str(KEY_CONFIG_WIFI_AP_SSID, (char *) config_ap.ap.ssid);

	snprintf((char *) config_ap.ap.password, sizeof(config_ap.ap.password), "server_pass");
	config_set_str(KEY_CONFIG_WIFI_AP_PASSWORD, (char *) config_ap.ap.password);
    config_ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_LOGI(TAG, "WIFI_AP_SSID: %s Password: %s", config_ap.ap.ssid,
	config_ap.ap.password);

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192,168,2,1);
    IP4_ADDR(&ipInfo.gw, 192,168,2,1);
    IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(esp_netif_ap);
    esp_netif_set_ip_info(esp_netif_ap, &ipInfo);
    esp_netif_dhcps_start(esp_netif_ap);
    ESP_LOGI(TAG, "WIFI_AP_IP: ip: " IPSTR "/%d, gw: " IPSTR,
	IP2STR(&ipInfo.ip),
	ffs(~ipInfo.netmask.addr) - 1,
	IP2STR(&ipInfo.gw));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());

}




void check_wifi_mode(bool sta_enable){


    	ESP_LOGI(TAG, "Check_wifi_mode Called");
    	if( sta_status_task != NULL )
    	{
    		vTaskDelete(sta_status_task);
    		vTaskDelete(sta_reconnect_task);
    		sta_status_task = NULL;
    		sta_reconnect_task = NULL;
    	}
		ESP_ERROR_CHECK(esp_wifi_disconnect());


		if (sta_enable) {


	        size_t sta_ssid_len = sizeof(config_sta.sta.ssid);
	        config_get_str(CONF_ITEM(KEY_CONFIG_WIFI_STA_SSID), &config_sta.sta.ssid, &sta_ssid_len);
	        sta_ssid_len--; // Remove null terminator from length

	        size_t sta_password_len = sizeof(config_sta.sta.password);
	        config_get_str(CONF_ITEM(KEY_CONFIG_WIFI_STA_PASSWORD), &config_sta.sta.password, &sta_password_len);
	        sta_password_len--; // Remove null terminator from length
	        config_sta.sta.scan_method = WIFI_FAST_SCAN;

	        ESP_LOGI(TAG, "WIFI_STA_CONNECTING: %s (%s), %s scan", config_sta.sta.ssid,
			sta_password_len == 0 ? "open" : "with password",
			config_sta.sta.scan_method == WIFI_ALL_CHANNEL_SCAN ? "all channel" : "fast");

	        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config_sta));
	        ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20));

	        ESP_ERROR_CHECK(esp_wifi_connect());

//	         Keep track of connection for RSSI indicator, but suspend until connected
	        xTaskCreate(wifi_sta_status_task, "wifi_sta_status", 2048, NULL, 0, &sta_status_task);
	        vTaskSuspend(sta_status_task);
//
	        // Reconnect when disconnected
	        xTaskCreate(wifi_sta_reconnect_task, "wifi_sta_reconnect", 4096, NULL, 0, &sta_reconnect_task);
	        vTaskSuspend(sta_reconnect_task);

	    }
}

wifi_sta_list_t *wifi_ap_sta_list() {

    esp_wifi_ap_get_sta_list(&ap_sta_list);
    return &ap_sta_list;
}

void wifi_ap_status(wifi_ap_status_t *status) {
    status->active = ap_active;
    if (!ap_active) return;

    memcpy(status->ssid, config_ap.ap.ssid, sizeof(config_ap.ap.ssid));
    status->authmode = config_ap.ap.authmode;

    wifi_ap_sta_list();
    status->devices = ap_sta_list.num;


    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_ap, &ip_info);
    status->ip4_addr = ip_info.ip;

    esp_netif_get_ip6_linklocal(esp_netif_ap, &status->ip6_addr);
}

void wifi_sta_status(wifi_sta_status_t *status) {
    status->active = sta_active;
    status->connected = sta_connected;
    if (!sta_connected) {
        memcpy(status->ssid, config_sta.sta.ssid, sizeof(config_sta.sta.ssid));
        return;
    }

    memcpy(status->ssid, sta_ap_info.ssid, sizeof(sta_ap_info.ssid));
    status->rssi = sta_ap_info.rssi;
    status->authmode = sta_ap_info.authmode;

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_sta, &ip_info);
    status->ip4_addr = ip_info.ip;

    esp_netif_get_ip6_linklocal(esp_netif_sta, &status->ip6_addr);
}

wifi_ap_record_t *wifi_scan(uint16_t *number) {
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);

    // Ensure STA is enabled
    if (wifi_mode != WIFI_MODE_APSTA && wifi_mode != WIFI_MODE_STA) {
        esp_wifi_set_mode(wifi_mode == WIFI_MODE_AP ? WIFI_MODE_APSTA : WIFI_MODE_STA);
    }

    wifi_scan_config_t wifi_scan_config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = 0
    };

    esp_wifi_scan_start(&wifi_scan_config, true);

    esp_wifi_scan_get_ap_num(number);
    if (*number <= 0) {
        return NULL;
    }

    wifi_ap_record_t *ap_records = (wifi_ap_record_t *) malloc(*number * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(number, ap_records);

    return ap_records;
}

const char *wifi_auth_mode_name(wifi_auth_mode_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2_ENTERPRISE";
        default:
            return "Unknown";
    }
}
