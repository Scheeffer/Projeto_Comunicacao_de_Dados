/*
 * ESP8266 (ESP3) - Atuador
 * Assina "atuador/comando" via MQTT. Ao receber "1" aciona o GPIO e
 * publica o estado ("LIGADO"/"DESLIGADO") em "status/atuador".
 * Framework: ESP8266 IOT RTOS SDK v1.5 (modelo user_init, C puro).
 *
 * Obs.: GPIO2 é pino de boot (deve estar em nível alto na partida).
 *       Acioná-lo após o boot é normal; ajuste ATUADOR_GPIO se necessário.
 */

#include <stdio.h>
#include <string.h>

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "esp8266/pin_mux_register.h"

#include "mqtt_min.h"

/* ── Configurações ─────────────────────────────────────── */
#define WIFI_SSID        "SUA_REDE_WIFI"
#define WIFI_PASS        "SUA_SENHA_WIFI"
#define BROKER_IP        "192.168.1.100"   /* IP do ESP32-S3 (broker) */
#define BROKER_PORTA     1883
#define TOPIC_ATUADOR    "atuador/comando"
#define TOPIC_STATUS     "status/atuador"
#define CLIENT_ID        "ESP8266_Atuador"
#define ATUADOR_GPIO     2
#define KEEPALIVE_S      60
#define RECV_TIMEOUT_MS  3000
#define PING_A_CADA      10                /* ~30s (10 x 3s de timeout) */
/* ──────────────────────────────────────────────────────── */

#ifndef UART_CLK_FREQ
#define UART_CLK_FREQ    (80 * 1000000)
#endif

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

    wifi_station_set_config(&cfg);
    wifi_station_set_reconnect_policy(1);
    wifi_set_event_handler_cb(wifi_event_cb);
}

/* ── Controle do atuador ──────────────────────────────── */

static void acionar(mqtt_client_t *client, int ligar)
{
    GPIO_OUTPUT_SET(ATUADOR_GPIO, ligar ? 1 : 0);
    const char *estado = ligar ? "LIGADO" : "DESLIGADO";
    mqtt_publish(client, TOPIC_STATUS, estado);
    printf("[ATUADOR] %s\n", estado);
}

/* ── Task principal ───────────────────────────────────── */

static void atuador_task(void *pv)
{
    mqtt_client_t client;
    client.sock = -1;
    int timeouts = 0;

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
            if (mqtt_subscribe(&client, TOPIC_ATUADOR) != 0) {
                printf("[MQTT] Falha ao assinar; reconectando\n");
                mqtt_close(&client);
                continue;
            }
            printf("[MQTT] Conectado; assinando \"%s\"\n", TOPIC_ATUADOR);
            timeouts = 0;
        }

        char topico[64];
        char payload[32];
        int r = mqtt_poll(&client, topico, sizeof(topico), payload, sizeof(payload));

        if (r < 0) {
            printf("[MQTT] Conexao perdida; reconectando\n");
            mqtt_close(&client);
            continue;
        }

        if (r == 1) {
            printf("[RECEBIDO] [%s]: \"%s\"\n", topico, payload);
            if (strcmp(topico, TOPIC_ATUADOR) == 0) {
                acionar(&client, strcmp(payload, "1") == 0 ? 1 : 0);
            }
            timeouts = 0;
        } else {
            /* timeout: mantém a conexão viva com PINGREQ periódico */
            if (++timeouts >= PING_A_CADA) {
                if (mqtt_ping(&client) != 0) {
                    printf("[MQTT] Falha no ping; reconectando\n");
                    mqtt_close(&client);
                }
                timeouts = 0;
            }
        }
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
    printf("\n[BOOT] Atuador MQTT - SDK %s\n", system_get_sdk_version());

    /* Configura o pino do atuador como saída, iniciando em nível baixo. */
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    GPIO_OUTPUT_SET(ATUADOR_GPIO, 0);

    wifi_init();
    xTaskCreate(atuador_task, "atuador_task", 2048, NULL, 4, NULL);
}
