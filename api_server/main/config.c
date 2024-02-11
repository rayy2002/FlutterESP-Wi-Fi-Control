
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <string.h>
#include <driver/uart.h>
#include <esp_wifi_types.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include "config.h"

static const char *TAG = "CONFIG";
static const char *STORAGE = "config";

nvs_handle_t config_handle;

const config_item_t CONFIG_ITEMS[] = {

        // WiFi
         {
                .key = KEY_CONFIG_WIFI_AP_SSID,
                .type = CONFIG_ITEM_TYPE_STRING,
                .def.str = "Esp_Server"
        }, {
                .key = KEY_CONFIG_WIFI_AP_PASSWORD,
                .type = CONFIG_ITEM_TYPE_STRING,
                .def.str = ""
        }, {
                .key = KEY_CONFIG_WIFI_STA_ACTIVE,
                .type = CONFIG_ITEM_TYPE_BOOL,
                .def.bool1 = true
        }, {
                .key = KEY_CONFIG_WIFI_STA_SSID,
                .type = CONFIG_ITEM_TYPE_STRING,
                .def.str = ""
        }, {
                .key = KEY_CONFIG_WIFI_STA_PASSWORD,
                .type = CONFIG_ITEM_TYPE_STRING,
                .def.str = ""
        }


};

const config_item_t *dev_config_items_get(int *count) {
    *count = sizeof(CONFIG_ITEMS) / sizeof(config_item_t);
    return &CONFIG_ITEMS[0];
}

esp_err_t config_set(const config_item_t *item, void *value) {
    switch (item->type) {
        case CONFIG_ITEM_TYPE_BOOL:
            return config_set_bool1(item->key, *((bool *) value));
        case CONFIG_ITEM_TYPE_STRING:
            return config_set_str(item->key, (char *) value);
        default:
            return ESP_ERR_INVALID_ARG;
    }
}



esp_err_t config_set_bool1(const char *key, bool value) {
    return nvs_set_i8(config_handle, key, value);
}

esp_err_t config_set_str(const char *key, char *value) {
    return nvs_set_str(config_handle, key, value);
}

esp_err_t config_set_blob(const char *key, char *value, size_t length) {
    return nvs_set_blob(config_handle, key, value, length);
}

esp_err_t config_init() {

	ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle '%s'... ", STORAGE);
    return nvs_open(STORAGE, NVS_READWRITE, &config_handle);
}

esp_err_t config_reset() {
    return nvs_erase_all(config_handle);
}



bool config_get_bool1(const config_item_t *item) {
    int8_t value = item->def.bool1;
    nvs_get_i8(config_handle, item->key, &value);
    return value > 0;
}

const config_item_t * config_get_item(const char *key) {
    for (unsigned int i = 0; i < sizeof(CONFIG_ITEMS) / sizeof(config_item_t); i++) {
        const config_item_t *item = &CONFIG_ITEMS[i];
        if (strcmp(item->key, key) == 0) {
            return item;
        }
    }

    // Fatal error
    ESP_ERROR_CHECK(ESP_FAIL);

    return NULL;
}

esp_err_t config_get_str(const config_item_t *item, void *out_value, size_t *length) {
    esp_err_t ret;

    switch (item->type) {
        case CONFIG_ITEM_TYPE_STRING:
            ret = nvs_get_str(config_handle, item->key, out_value, length);
            if (ret == ESP_ERR_NVS_NOT_FOUND) {
                if (length != NULL) *length = strlen(item->def.str) + 1;
                if (out_value != NULL) strcpy(out_value, item->def.str);
            }
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    return (ret == ESP_OK || ret == ESP_ERR_NVS_NOT_FOUND) ? ESP_OK : ret;
}

esp_err_t config_commit() {
    return nvs_commit(config_handle);
}

static void config_restart_task() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void config_restart() {
    xTaskCreate(config_restart_task, "config_restart_task", 4096, NULL, 100, NULL);
}
