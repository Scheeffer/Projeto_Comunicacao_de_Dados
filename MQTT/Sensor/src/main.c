/*
 * ESP8266 (ESP2) - Sensor
 * Lê ADC e publica o valor em MQTT para o broker (ESP32S3).
 * Framework: ESP8266 RTOS SDK (C puro)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/adc.h"

/* ── Configurações ─────────────────────────────────────── */
#define WIFI_SSID        "SUA_REDE_WIFI"
#define WIFI_PASS        "SUA_SENHA_WIFI"
#define BROKER_URI       "mqtt://192.168.1.100:1883"  /* IP do ESP32S3 */
#define TOPIC_SENSOR     "sensor/dados"
#define PUB_INTERVALO_MS 5000
/* ──────────────────────────────────────────────────────── */

static const char *TAG = "SENSOR";

static EventGroupHandle_t       s_wifi_eg;
#define WIFI_CONECTADO_BIT      BIT0

static esp_mqtt_client_handle_t s_mqtt    = NULL;
static volatile int             s_mqtt_ok = 0;

/* ── Handlers ─────────────────────────────────────────── */

static void wifi_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_mqtt_ok = 0;
        esp_wifi_connect();
        ESP_LOGW(TAG, "WiFi desconectado, reconectando...");

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONECTADO_BIT);
    }
}

static void mqtt_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT conectado ao broker");
        s_mqtt_ok = 1;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT desconectado");
        s_mqtt_ok = 0;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Erro MQTT");
        break;
    default:
        break;
    }
}

/* ── Inicialização ────────────────────────────────────── */

static void wifi_init(void)
{
    s_wifi_eg = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,  wifi_handler, NULL);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, wifi_handler, NULL);

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    strncpy((char *)wifi_cfg.sta.ssid,     WIFI_SSID, sizeof(wifi_cfg.sta.ssid)     - 1);
    strncpy((char *)wifi_cfg.sta.password, WIFI_PASS, sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(s_wifi_eg, WIFI_CONECTADO_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.uri       = BROKER_URI;
    cfg.client_id = "ESP8266_Sensor";

    s_mqtt = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, mqtt_handler, NULL);
    esp_mqtt_client_start(s_mqtt);
}

/* ── Task de leitura e publicação ─────────────────────── */

static void sensor_task(void *pv)
{
    /* ADC no modo TOUT (pino A0), divisor de clock 8 */
    adc_config_t adc_cfg = {
        .mode    = ADC_READ_TOUT_MODE,
        .clk_div = 8,
    };
    adc_init(&adc_cfg);

    for (;;) {
        if (s_mqtt_ok) {
            uint16_t raw = 0;
            adc_read(&raw);

            /* O ADC do ESP8266 tem referência de 1,0 V (0-1023).
             * Ajuste o cálculo caso seu hardware tenha divisor de tensão. */
            float tensao = raw * (1.0f / 1023.0f);

            char payload[32];
            snprintf(payload, sizeof(payload), "%.3f", tensao);

            int msg_id = esp_mqtt_client_publish(s_mqtt, TOPIC_SENSOR,
                                                 payload, 0, 1, 0);
            ESP_LOGI(TAG, "Publicado [%s] -> \"%s\" (msg_id=%d)",
                     TOPIC_SENSOR, payload, msg_id);
        } else {
            ESP_LOGD(TAG, "MQTT indisponível, aguardando...");
        }

        vTaskDelay(pdMS_TO_TICKS(PUB_INTERVALO_MS));
    }
}

/* ── Entry point ──────────────────────────────────────── */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    mqtt_init();
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
