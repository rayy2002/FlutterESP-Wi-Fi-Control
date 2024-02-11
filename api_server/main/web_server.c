#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "esp_system.h"
#include <string.h>
#include "esp_log.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <wifi.h>
#include <cJSON.h>
#include <config.h>
#include <lwip/inet.h>
#include <esp_netif_sta_list.h>
#include "web_server.h"

static const char *TAG = "WEB";

static httpd_handle_t xServer = NULL;

static char *buffer;
#define BUFFER_SIZE 2048




static esp_err_t json_response(httpd_req_t *req, cJSON *root) {
    // Set mime type
    esp_err_t err = httpd_resp_set_type(req, "application/json");
    if (err != ESP_OK) return err;

    // Convert to string
    bool success = cJSON_PrintPreallocated(root, buffer, BUFFER_SIZE, false);
    cJSON_Delete(root);
    if (!success) {
        ESP_LOGE(TAG, "Not enough space in buffer to output JSON");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not enough space in buffer to output JSON");
        return ESP_FAIL;
    }

    // Send as response
    err = httpd_resp_send(req, buffer, strlen(buffer));
    if (err != ESP_OK) return err;

    return ESP_OK;
}

static esp_err_t register_uri_handler(httpd_handle_t server, const char *path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r)) {
    httpd_uri_t uri_config_get = {
            .uri        = path,
            .method     = method,
            .handler    = handler
    };
    return httpd_register_uri_handler(server, &uri_config_get);
}

static esp_err_t status_get_handler(httpd_req_t *req) {

    cJSON *root = cJSON_CreateArray();

    // WiFi
    wifi_sta_status_t sta_status;
    wifi_sta_status(&sta_status);

    cJSON *sta = cJSON_CreateObject();
    cJSON_AddItemToArray(root, sta);
    cJSON_AddBoolToObject(sta, "active", sta_status.active);
    if (sta_status.active) {
        cJSON_AddBoolToObject(sta, "connected", sta_status.connected);
        if (sta_status.connected) {
            cJSON_AddStringToObject(sta, "ssid", (char *) sta_status.ssid);
            cJSON_AddStringToObject(sta, "authmode", wifi_auth_mode_name(sta_status.authmode));
            cJSON_AddNumberToObject(sta, "rssi", sta_status.rssi);

            char ip[40];
            snprintf(ip, sizeof(ip), IPSTR, IP2STR(&sta_status.ip4_addr));
            cJSON_AddStringToObject(sta, "ip4", ip);
            snprintf(ip, sizeof(ip), IPV6STR, IPV62STR(sta_status.ip6_addr));
            cJSON_AddStringToObject(sta, "ip6", ip);
        }
    }

    return json_response(req, root);
}

static esp_err_t wifi_scan_get_handler(httpd_req_t *req) {

	ESP_LOGI("Web Server", "WiFi Scan Handler");
    uint16_t ap_count;
    wifi_ap_record_t *ap_records =  wifi_scan(&ap_count);

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++) {
        wifi_ap_record_t *ap_record = &ap_records[i];
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddItemToArray(root, ap);
        cJSON_AddStringToObject(ap, "ssid", (char *) ap_record->ssid);
        cJSON_AddNumberToObject(ap, "rssi", ap_record->rssi);
        cJSON_AddStringToObject(ap, "authmode", wifi_auth_mode_name(ap_record->authmode));
    }

    free(ap_records);

    return json_response(req, root);
}


static esp_err_t config_post_handler(httpd_req_t *req) {

    int ret = httpd_req_recv(req, buffer, BUFFER_SIZE - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }

        return ESP_FAIL;
    }

    buffer[ret] = '\0';

    cJSON *root = cJSON_Parse(buffer);

    int config_item_count;
    const config_item_t *config_items = dev_config_items_get(&config_item_count);
    for (int i = 0; i < config_item_count; i++) {
        config_item_t item = config_items[i];

        if (cJSON_HasObjectItem(root, item.key)) {
            cJSON *entry = cJSON_GetObjectItem(root, item.key);
            esp_err_t err;
            if (item.type == CONFIG_ITEM_TYPE_STRING) {
				err = config_set_str(item.key, entry->valuestring);
            }
            else {
                int64_t int64 = strtol(entry->valuestring, NULL, 10);
                err = config_set(&item, &int64);
            }
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error setting %s = %s: %d - %s", item.key, entry->valuestring, err, esp_err_to_name(err));
            }
        }
    }

    cJSON_Delete(root);

    config_commit();

    root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);

    return json_response(req, root);
}

static esp_err_t wifi_post_handler(httpd_req_t *req){

	config_post_handler(req);
	check_wifi_mode(0);

	if (config_get_bool1(CONF_ITEM(KEY_CONFIG_WIFI_STA_ACTIVE)))
	{
		check_wifi_mode(1);
	}
	cJSON *root = cJSON_CreateObject();
	cJSON_AddBoolToObject(root, "success", true);

	return json_response(req, root);

}

void StartWebServer()
{
	if(xServer != NULL)
		return;


	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 20;
	config.uri_match_fn = httpd_uri_match_wildcard;
	if(httpd_start(&xServer, &config) == ESP_OK)
	{

		register_uri_handler(xServer, DEVICE_STATUS_URI, HTTP_GET, status_get_handler);
		register_uri_handler(xServer, WIFI_SCAN_URL, HTTP_GET, wifi_scan_get_handler);
		register_uri_handler(xServer, WIFI_CONFIG_URL, HTTP_POST, wifi_post_handler);

	}

	buffer = malloc(BUFFER_SIZE);

}
