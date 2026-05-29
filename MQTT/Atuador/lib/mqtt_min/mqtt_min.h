/*
 * mqtt_min - cliente MQTT 3.1.1 mínimo (QoS 0) sobre sockets lwIP.
 *
 * Implementa apenas o necessário para falar com o broker artesanal do
 * ESP32-S3 (CONNECT/CONNACK, PUBLISH, SUBSCRIBE/SUBACK, PINGREQ/PINGRESP,
 * DISCONNECT). Sem dependências do esp-mqtt; compila no ESP8266 RTOS SDK v1.5.
 */

#ifndef MQTT_MIN_H
#define MQTT_MIN_H

#include <stdint.h>

typedef struct {
    int sock;   /* descritor TCP; -1 quando desconectado */
} mqtt_client_t;

/*
 * Abre conexão TCP, envia CONNECT e aguarda CONNACK.
 *  recv_timeout_ms: timeout de leitura do socket (>0). Usado também como
 *                   limite de espera do CONNACK e do mqtt_poll().
 * Retorna 0 em sucesso, -1 em falha (socket é fechado em caso de erro).
 */
int mqtt_connect(mqtt_client_t *c, const char *broker_ip, uint16_t porta,
                 const char *client_id, uint16_t keepalive_s,
                 int recv_timeout_ms);

/* Publica payload (string) em topico com QoS 0. 0 = ok, -1 = falha. */
int mqtt_publish(mqtt_client_t *c, const char *topico, const char *payload);

/* Assina topico com QoS 0. 0 = ok, -1 = falha. */
int mqtt_subscribe(mqtt_client_t *c, const char *topico);

/* Envia PINGREQ (keep-alive). 0 = ok, -1 = falha. */
int mqtt_ping(mqtt_client_t *c);

/*
 * Lê um pacote do broker (respeitando o recv_timeout configurado).
 *   1  : PUBLISH recebido; 'topico' e 'payload' preenchidos (terminados em \0).
 *   0  : timeout ou pacote irrelevante (SUBACK/PINGRESP) — repita a chamada.
 *  -1  : conexão perdida.
 */
int mqtt_poll(mqtt_client_t *c, char *topico, int topico_sz,
              char *payload, int payload_sz);

/* Envia DISCONNECT (best-effort) e fecha o socket. */
void mqtt_close(mqtt_client_t *c);

#endif /* MQTT_MIN_H */
