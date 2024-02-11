#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    CONFIG_ITEM_TYPE_BOOL = 0,
    CONFIG_ITEM_TYPE_STRING,
} config_item_type_t;


typedef union {
    bool bool1;
    char *str;

} config_item_value_t;

typedef struct config_item {
    char *key;
    config_item_type_t type;
    config_item_value_t def;
} config_item_t;


// WiFi
#define KEY_CONFIG_WIFI_AP_SSID "w_ap_ssid"
#define KEY_CONFIG_WIFI_AP_PASSWORD "w_ap_pass"
#define KEY_CONFIG_WIFI_STA_ACTIVE "w_sta_active"
#define KEY_CONFIG_WIFI_STA_SSID "w_sta_ssid"
#define KEY_CONFIG_WIFI_STA_PASSWORD "w_sta_pass"

esp_err_t config_init();
esp_err_t config_reset();

const config_item_t *dev_config_items_get(int *count);
const config_item_t *config_get_item(const char *key);

#define CONF_ITEM( key ) config_get_item(key)

bool config_get_bool1(const config_item_t *item);


esp_err_t config_set(const config_item_t *item, void *value);
esp_err_t config_set_bool1(const char *key, bool value);
esp_err_t config_set_str(const char *key, char *value);
esp_err_t config_get_str(const config_item_t *item, void *out_value, size_t *length);
esp_err_t config_commit();

void config_restart();

#endif //CONFIG_H
