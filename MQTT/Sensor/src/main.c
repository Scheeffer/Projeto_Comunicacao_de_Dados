/*
 * ESP8266 (ESP2) - Sensor
 * Lê o ADC (TOUT/A0) e publica a tensão em MQTT para o broker (ESP32-S3).
 * Framework: ESP8266 IOT RTOS SDK v1.5 (modelo user_init, C puro).
 *
 * Obs.: a placa esp01_1m não expõe fisicamente o pino ADC. Em hardware sem
 *       ADC acessível, system_adc_read() ainda retorna um valor (ruído).
 */

#include <stdio.h>
#include <string.h>

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_min.h"

/* ── Configurações ─────────────────────────────────────── */
#define WIFI_SSID        "SUA_REDE_WIFI"
#define WIFI_PASS        "SUA_SENHA_WIFI"
#define BROKER_IP        "192.168.1.100"   /* IP do ESP32-S3 (broker) */
#define BROKER_PORTA     1883
#define TOPIC_SENSOR     "sensor/dados"
#define CLIENT_ID        "ESP8266_Sensor"
#define KEEPALIVE_S      60
#define RECV_TIMEOUT_MS  4000
#define PUB_INTERVALO_MS 5000
/* ──────────────────────────────────────────────────────── */

#ifndef UART_CLK_FREQ
#define UART_CLK_FREQ    (80 * 1000000)
#endif

/* Função ROM para ajustar o baud rate da UART0 */
extern void uart_div_modify(uint8 uart_no, uint32 div);

static volatile int s_got_ip = 0;

/* ── Handler de eventos WiFi ──────────────────────────── */

static void wifi_event_cb(System_Event_t *evt)
{
    switch (evt->event_id) {
    case EVENT_STAMODE_GOT_IP:
        printf("[WIFI] IP obtido\n");
        s_got_ip = 1;
        break;
    case EVENT_STAMODE_DISCONNECTED:
        printf("[WIFI] Desconectado, reconectando...\n");
        s_got_ip = 0;
        break;
    default:
        break;
    }
}

static void wifi_init(void)
{
    wifi_set_opmode(STATION_MODE);

    struct station_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy((char *)cfg.ssid,     WIFI_SSID, sizeof(cfg.ssid)     - 1);
    strncpy((char *)cfg.password, WIFI_PASS, sizeof(cfg.password) - 1);
    cfg.bssid_set = 0;

    /* Em user_init, set_config dispara a conexão automaticamente. */
    wifi_station_set_config(&cfg);
    wifi_station_set_reconnect_policy(1);
    wifi_set_event_handler_cb(wifi_event_cb);
}

/* ── Task de leitura e publicação ─────────────────────── */

static void sensor_task(void *pv)
{
    mqtt_client_t client;
    client.sock = -1;

    for (;;) {
        if (!s_got_ip) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        if (client.sock < 0) {
            printf("[MQTT] Conectando ao broker %s:%d ...\n", BROKER_IP, BROKER_PORTA);
            if (mqtt_connect(&client, BROKER_IP, BROKER_PORTA,
                             CLIENT_ID, KEEPALIVE_S, RECV_TIMEOUT_MS) != 0) {
                printf("[MQTT] Falha na conexao; nova tentativa em 3s\n");
                vTaskDelay(3000 / portTICK_RATE_MS);
                continue;
            }
            printf("[MQTT] Conectado ao broker\n");
        }

        /* ADC do ESP8266: 0-1023, referência interna de 1,0 V. */
        uint16 raw = system_adc_read();
        float  tensao = raw * (1.0f / 1023.0f);

        char payload[32];
        snprintf(payload, sizeof(payload), "%.3f", tensao);

        if (mqtt_publish(&client, TOPIC_SENSOR, payload) != 0) {
            printf("[MQTT] Falha ao publicar; reconectando\n");
            mqtt_close(&client);
            continue;
        }
        printf("[SENSOR] [%s] -> \"%s\" (raw=%u)\n", TOPIC_SENSOR, payload, raw);

        vTaskDelay(PUB_INTERVALO_MS / portTICK_RATE_MS);
    }
}

/* ── Boilerplate obrigatório do SDK v1.5 ──────────────── */

uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:   rf_cal_sec = 128 - 5;  break;
        case FLASH_SIZE_8M_MAP_512_512:   rf_cal_sec = 256 - 5;  break;
        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024: rf_cal_sec = 512 - 5;  break;
        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024: rf_cal_sec = 1024 - 5; break;
        case FLASH_SIZE_64M_MAP_1024_1024: rf_cal_sec = 2048 - 5; break;
        case FLASH_SIZE_128M_MAP_1024_1024: rf_cal_sec = 4096 - 5; break;
        default:                          rf_cal_sec = 0;        break;
    }
    return rf_cal_sec;
}

/* ── Entry point ──────────────────────────────────────── */

void user_init(void)
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    printf("\n[BOOT] Sensor MQTT - SDK %s\n", system_get_sdk_version());

    wifi_init();
    xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 4, NULL);
}
