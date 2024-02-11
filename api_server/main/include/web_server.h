

#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#define INDEX_PAGE_URL "/"
#define SERVER_URL INDEX_PAGE_URL "esp_server"
#define DEVICE_STATUS_URI SERVER_URL "/device_status"
#define WIFI_SCAN_URL SERVER_URL "/wifi_scan"
#define WIFI_CONFIG_URL SERVER_URL "/wifi_config"




void StartWebServer();



#endif /* WEB_SERVER_H_ */
