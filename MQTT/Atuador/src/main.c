/*
 * ESP8266 (ESP3) - Atuador
 * Assina "atuador/comando" via MQTT. Ao receber "1" aciona o GPIO
 * e publica o estado em "status/atuador".
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
#include "driver/gpio.h"

/* ── Configurações ─────────────────────────────────────── */
#define WIFI_SSID        "SUA_REDE_WIFI"
#define WIFI_PASS        "SUA_SENHA_WIFI"
#define BROKER_URI       "mqtt://192.168.1.100:1883"  /* IP do ESP32S3 */
#define TOPIC_ATUADOR    "atuador/comando"
#define TOPIC_STATUS     "status/atuador"
#define ATUADOR_GPIO     2    /* GPIO2 = D4 no NodeMCU; ajuste se necessário */
/* ──────────────────────────────────────────────────────── */

static const char *TAG = "ATUADOR";

static EventGroupHandle_t       s_wifi_eg;
#define WIFI_CONECTADO_BIT      BIT0

static esp_mqtt_client_handle_t s_mqtt = NULL;

/* ── Controle do atuador ──────────────────────────────── */

static void acionar(int ligar)
{
    gpio_set_level(ATUADOR_GPIO, ligar);
    const char *estado = ligar ? "LIGADO" : "DESLIGADO";
    esp_mqtt_client_publish(s_mqtt, TOPIC_STATUS, estado, 0, 1, 0);
    ESP_LOGI(TAG, "Atuador: %s", estado);
}

/* ── Handlers ─────────────────────────────────────────── */

static void mqtt_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)data;

    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT conectado ao broker");
        esp_mqtt_client_subscribe(s_mqtt, TOPIC_ATUADOR, 1);
        ESP_LOGI(TAG, "Assinando: %s", TOPIC_ATUADOR);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT desconectado");
        break;

    case MQTT_EVENT_DATA: {
        /* Copia tópico e payload com terminador '\0' */
        char topico[64]  = {0};
        char payload[32] = {0};
        int tlen = ev->topic_len < (int)sizeof(topico)  - 1
                   ? ev->topic_len : (int)sizeof(topico)  - 1;
        int plen = ev->data_len  < (int)sizeof(payload) - 1
                   ? ev->data_len  : (int)sizeof(payload) - 1;
        memcpy(topico,  ev->topic, tlen);
        memcpy(payload, ev->data,  plen);

        ESP_LOGI(TAG, "[RECEBIDO] %s: %s", topico, payload);

        if (strcmp(topico, TOPIC_ATUADOR) == 0) {
            acionar(strcmp(payload, "1") == 0 ? 1 : 0);
        }
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Erro MQTT");
        break;

    default:
        break;
    }
}

static void wifi_handler(void *arg, esp_event_base_t base,
                         int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGW(TAG, "WiFi desconectado, reconectando...");

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONECTADO_BIT);
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

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    wifi_handler, NULL);
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
    cfg.client_id = "ESP8266_Atuador";

    s_mqtt = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, mqtt_handler, NULL);
    esp_mqtt_client_start(s_mqtt);
}

/* ── Entry point ──────────────────────────────────────── */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Configura o pino do atuador como saída e garante estado inicial OFF */
    gpio_set_direction(ATUADOR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ATUADOR_GPIO, 0);

    wifi_init();
    mqtt_init();
    /* Toda a lógica é orientada a eventos via mqtt_handler */
}
