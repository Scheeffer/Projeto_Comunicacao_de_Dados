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

/*
 * As credenciais reais NAO ficam aqui.
 * Copie main/wifi_secrets.example.h para main/wifi_secrets.h e preencha.
 * O arquivo wifi_secrets.h esta no .gitignore e nunca deve ser commitado.
 */
#include "wifi_secrets.h"

static const char *TAG = "WIFI";

void wifi_init_sta(void);

#endif
