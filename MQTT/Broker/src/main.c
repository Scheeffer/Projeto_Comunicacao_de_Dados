/*
 * ESP32S3 (ESP1) - Broker MQTT
 * Implementa um broker MQTT mínimo via TCP (QoS 0) usando lwIP.
 * Lógica: ao receber "sensor/dados", avalia o valor e publica
 *         "1" ou "0" em "atuador/comando" para o ESP3.
 * Framework: ESP-IDF (C puro)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

/* ── Configurações ─────────────────────────────────────── */
#define WIFI_SSID        "SUA_REDE_WIFI"
#define WIFI_PASS        "SUA_SENHA_WIFI"
#define MQTT_PORTA       1883
#define MAX_CLIENTES     5
#define MAX_ASSINATURAS  20
#define TOPIC_SENSOR     "sensor/dados"
#define TOPIC_ATUADOR    "atuador/comando"
#define TOPIC_STATUS     "status/atuador"
#define LIMIAR           2.5f   /* tensão em volts para acionar o atuador */
/* ──────────────────────────────────────────────────────── */

static const char *TAG = "BROKER_MQTT";

/* ── Tabela de assinaturas ────────────────────────────── */
typedef struct {
    int  sock;          /* -1 = slot livre */
    char topico[64];
} assinatura_t;

static assinatura_t      s_subs[MAX_ASSINATURAS];
static SemaphoreHandle_t s_mutex;

/* ── Helpers de socket ────────────────────────────────── */

static int recv_exato(int sock, uint8_t *buf, int n)
{
    int total = 0;
    while (total < n) {
        int r = recv(sock, buf + total, n - total, 0);
        if (r <= 0) return -1;
        total += r;
    }
    return total;
}

/* Decodifica o campo "Remaining Length" do MQTT (base-128) */
static int decodificar_tamanho(int sock, int *out)
{
    int mult = 1, len = 0;
    uint8_t b;
    do {
        if (recv_exato(sock, &b, 1) < 0) return -1;
        len  += (b & 0x7F) * mult;
        mult <<= 7;
        if (mult > 128 * 128 * 128) return -1;
    } while (b & 0x80);
    *out = len;
    return 0;
}

/* Codifica o campo "Remaining Length" em buf[], retorna bytes usados */
static int codificar_tamanho(uint8_t *buf, int len)
{
    int i = 0;
    do {
        buf[i] = len & 0x7F;
        len  >>= 7;
        if (len > 0) buf[i] |= 0x80;
        i++;
    } while (len > 0);
    return i;
}

/* ── Envio de pacotes MQTT ────────────────────────────── */

static void enviar_connack(int sock)
{
    /* CONNACK: tipo=0x20, tamanho=2, session_present=0, return_code=0 */
    uint8_t pkt[] = {0x20, 0x02, 0x00, 0x00};
    send(sock, pkt, sizeof(pkt), 0);
}

static void enviar_suback(int sock, uint16_t pkt_id)
{
    /* SUBACK: tipo=0x90, tamanho=3, packet_id, QoS_concedido=0 */
    uint8_t pkt[] = {0x90, 0x03,
                     (uint8_t)(pkt_id >> 8), (uint8_t)(pkt_id & 0xFF),
                     0x00};
    send(sock, pkt, sizeof(pkt), 0);
}

static void enviar_pingresp(int sock)
{
    uint8_t pkt[] = {0xD0, 0x00};
    send(sock, pkt, sizeof(pkt), 0);
}

/* Publica uma mensagem QoS-0 diretamente em um socket */
static void publicar_para_sock(int sock, const char *topico, const char *payload)
{
    uint16_t tlen  = (uint16_t)strlen(topico);
    uint16_t plen  = (uint16_t)strlen(payload);
    int      total = 2 + tlen + plen;

    uint8_t rl[4];
    int     rl_n = codificar_tamanho(rl, total);

    uint8_t fixed = 0x30; /* PUBLISH, QoS=0, retain=0, dup=0 */
    send(sock, &fixed, 1, 0);
    send(sock, rl, rl_n, 0);

    uint8_t th[2] = {(uint8_t)(tlen >> 8), (uint8_t)(tlen & 0xFF)};
    send(sock, th, 2, 0);
    send(sock, topico,  tlen, 0);
    send(sock, payload, plen, 0);
}

/* ── Gestão de assinaturas ────────────────────────────── */

static void adicionar_assinatura(int sock, const char *topico)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_ASSINATURAS; i++) {
        if (s_subs[i].sock < 0) {
            s_subs[i].sock = sock;
            strncpy(s_subs[i].topico, topico, sizeof(s_subs[i].topico) - 1);
            ESP_LOGI(TAG, "Assinatura registrada: sock=%d topico=%s", sock, topico);
            break;
        }
    }
    xSemaphoreGive(s_mutex);
}

static void remover_assinaturas(int sock)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_ASSINATURAS; i++) {
        if (s_subs[i].sock == sock) {
            s_subs[i].sock     = -1;
            s_subs[i].topico[0] = '\0';
        }
    }
    xSemaphoreGive(s_mutex);
}

/* Encaminha payload para todos os assinantes de um tópico */
static void encaminhar(const char *topico, const char *payload)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_ASSINATURAS; i++) {
        if (s_subs[i].sock >= 0 && strcmp(s_subs[i].topico, topico) == 0) {
            publicar_para_sock(s_subs[i].sock, topico, payload);
        }
    }
    xSemaphoreGive(s_mutex);
}

/* ── Lógica de controle ───────────────────────────────── */

static void processar_sensor(const char *payload)
{
    float valor = strtof(payload, NULL);
    ESP_LOGI(TAG, "Sensor: %.3f V (limiar: %.2f V)", valor, LIMIAR);

    const char *cmd = (valor > LIMIAR) ? "1" : "0";
    ESP_LOGI(TAG, "Comando -> [%s]: %s", TOPIC_ATUADOR, cmd);
    encaminhar(TOPIC_ATUADOR, cmd);
}

/* ── Task por cliente MQTT ────────────────────────────── */

static void cliente_task(void *arg)
{
    int     sock = (int)(intptr_t)arg;
    uint8_t buf[512];

    for (;;) {
        /* Lê fixed header (1 byte) */
        uint8_t cmd_byte;
        if (recv_exato(sock, &cmd_byte, 1) < 0) break;

        /* Lê remaining length */
        int restante;
        if (decodificar_tamanho(sock, &restante) < 0) break;

        /* Lê o payload completo */
        if (restante > (int)sizeof(buf)) break;
        if (restante > 0 && recv_exato(sock, buf, restante) < 0) break;

        uint8_t tipo = cmd_byte & 0xF0;

        /* ── CONNECT (0x10) ── */
        if (tipo == 0x10) {
            enviar_connack(sock);
            ESP_LOGI(TAG, "CONNECT -> CONNACK (sock=%d)", sock);

        /* ── PUBLISH (0x30) ── */
        } else if (tipo == 0x30) {
            uint8_t qos = (cmd_byte >> 1) & 0x03;
            int pos = 0;

            uint16_t tlen = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
            pos += 2;

            char topico[64]   = {0};
            int  cpy = (int)tlen < (int)sizeof(topico) - 1
                       ? (int)tlen : (int)sizeof(topico) - 1;
            memcpy(topico, buf + pos, cpy);
            pos += tlen;

            if (qos > 0) pos += 2; /* pular Packet Identifier */

            int  plen   = restante - 2 - tlen - (qos > 0 ? 2 : 0);
            char payload[256] = {0};
            if (plen > 0 && plen < (int)sizeof(payload))
                memcpy(payload, buf + pos, plen);

            ESP_LOGI(TAG, "PUBLISH [%s]: %s", topico, payload);

            /* Encaminha para assinantes do tópico publicado */
            encaminhar(topico, payload);

            /* Aplica lógica de controle */
            if (strcmp(topico, TOPIC_SENSOR) == 0) {
                processar_sensor(payload);
            } else if (strcmp(topico, TOPIC_STATUS) == 0) {
                ESP_LOGI(TAG, "Status atuador: %s", payload);
            }

        /* ── SUBSCRIBE (0x80) ── */
        } else if (tipo == 0x80) {
            uint16_t pkt_id = ((uint16_t)buf[0] << 8) | buf[1];
            int pos = 2;
            while (pos < restante) {
                uint16_t tlen = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
                pos += 2;
                char topico[64] = {0};
                int  cpy = (int)tlen < (int)sizeof(topico) - 1
                           ? (int)tlen : (int)sizeof(topico) - 1;
                memcpy(topico, buf + pos, cpy);
                pos += tlen + 1; /* +1 = byte de QoS pedido */
                adicionar_assinatura(sock, topico);
            }
            enviar_suback(sock, pkt_id);

        /* ── PINGREQ (0xC0) ── */
        } else if (tipo == 0xC0) {
            enviar_pingresp(sock);

        /* ── DISCONNECT (0xE0) ── */
        } else if (tipo == 0xE0) {
            break;

        } else {
            ESP_LOGW(TAG, "Pacote desconhecido: 0x%02X", cmd_byte);
        }
    }

    ESP_LOGI(TAG, "Cliente desconectado (sock=%d)", sock);
    remover_assinaturas(sock);
    close(sock);
    vTaskDelete(NULL);
}

/* ── Servidor TCP (porta 1883) ────────────────────────── */

static void broker_task(void *arg)
{
    int srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(MQTT_PORTA);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(srv, (struct sockaddr *)&addr, sizeof(addr));
    listen(srv, MAX_CLIENTES);

    ESP_LOGI(TAG, "Broker aguardando conexões na porta %d", MQTT_PORTA);

    for (;;) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cli = accept(srv, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli < 0) continue;

        ESP_LOGI(TAG, "Novo cliente: %s (sock=%d)",
                 inet_ntoa(cli_addr.sin_addr), cli);
        xTaskCreate(cliente_task, "mqtt_cli", 4096,
                    (void *)(intptr_t)cli, 5, NULL);
    }
}

/* ── WiFi ─────────────────────────────────────────────── */

static EventGroupHandle_t s_wifi_eg;
#define WIFI_CONECTADO_BIT BIT0

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
        ESP_LOGI(TAG, "IP do Broker: " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(s_wifi_eg, WIFI_CONECTADO_BIT);
    }
}

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

/* ── Entry point ──────────────────────────────────────── */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Inicializa tabela de assinaturas */
    s_mutex = xSemaphoreCreateMutex();
    for (int i = 0; i < MAX_ASSINATURAS; i++) {
        s_subs[i].sock      = -1;
        s_subs[i].topico[0] = '\0';
    }

    wifi_init();
    xTaskCreate(broker_task, "broker_task", 8192, NULL, 5, NULL);
}
