#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SSID      "Lucas"/*"POCO X7"*/
#define WIFI_PASSWORD  "asenhae2025"/*"pzmy6ydqjhax566"*/

static const char *TAG = "WIFI";

void wifi_init_sta(void);

#endif