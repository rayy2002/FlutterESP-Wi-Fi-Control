#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "wifi.h"
#include "config.h"
#include "web_server.h"

void app_main(void)
{
	config_init();
	wifi_init();
	StartWebServer();
}
