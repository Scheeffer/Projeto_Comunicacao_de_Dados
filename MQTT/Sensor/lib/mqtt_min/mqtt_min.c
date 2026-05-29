/*
 * mqtt_min - implementação. Ver mqtt_min.h.
 * Compatível com o broker do ESP32-S3 (QoS 0, MQTT 3.1.1).
 */

#include "mqtt_min.h"

#include <string.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"

/* Tipos de pacote MQTT (nibble alto do byte fixo) */
#define MQTT_CONNECT     0x10
#define MQTT_CONNACK     0x20
#define MQTT_PUBLISH     0x30
#define MQTT_SUBSCRIBE   0x82   /* 0x80 | flags 0x02 obrigatório */
#define MQTT_SUBACK      0x90
#define MQTT_PINGREQ     0xC0
#define MQTT_PINGRESP    0xD0
#define MQTT_DISCONNECT  0xE0

/* ── Helpers de socket ────────────────────────────────── */

/* Envia o buffer inteiro. 0 = ok, -1 = falha. */
static int send_all(int sock, const uint8_t *buf, int n)
{
    int total = 0;
    while (total < n) {
        int w = send(sock, buf + total, n - total, 0);
        if (w <= 0) return -1;
        total += w;
    }
    return 0;
}

/* Lê exatamente n bytes (usado quando o pacote já está em trânsito).
 * 0 = ok, -1 = falha/timeout. */
static int recv_all(int sock, uint8_t *buf, int n)
{
    int total = 0;
    while (total < n) {
        int r = recv(sock, buf + total, n - total, 0);
        if (r <= 0) return -1;
        total += r;
    }
    return 0;
}

/* Lê o primeiro byte (cabeçalho fixo) respeitando o timeout.
 *  1 = byte lido, 0 = timeout (sem dados), -1 = conexão fechada/erro. */
static int recv_header(int sock, uint8_t *b)
{
    int r = recv(sock, b, 1, 0);
    if (r == 1) return 1;
    if (r < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) return 0;
    return -1;
}

/* Decodifica "Remaining Length" (base-128). 0 = ok, -1 = falha. */
static int recv_remaining_len(int sock, int *out)
{
    int mult = 1, len = 0;
    uint8_t b;
    do {
        if (recv_all(sock, &b, 1) < 0) return -1;
        len  += (b & 0x7F) * mult;
        mult <<= 7;
        if (mult > 128 * 128 * 128) return -1;
    } while (b & 0x80);
    *out = len;
    return 0;
}

/* Codifica "Remaining Length" em buf; retorna nº de bytes usados (1..4). */
static int encode_remaining_len(uint8_t *buf, int len)
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

/* Monta e envia um pacote: byte fixo + remaining length + corpo. */
static int send_packet(int sock, uint8_t fixed, const uint8_t *body, int body_len)
{
    uint8_t hdr[5];
    hdr[0] = fixed;
    int n = 1 + encode_remaining_len(hdr + 1, body_len);

    if (send_all(sock, hdr, n) < 0) return -1;
    if (body_len > 0 && send_all(sock, body, body_len) < 0) return -1;
    return 0;
}

/* Descarta n bytes do socket (pacote grande demais para o buffer). */
static int drain(int sock, int n)
{
    uint8_t tmp[64];
    while (n > 0) {
        int chunk = n < (int)sizeof(tmp) ? n : (int)sizeof(tmp);
        if (recv_all(sock, tmp, chunk) < 0) return -1;
        n -= chunk;
    }
    return 0;
}

/* ── API pública ──────────────────────────────────────── */

int mqtt_connect(mqtt_client_t *c, const char *broker_ip, uint16_t porta,
                 const char *client_id, uint16_t keepalive_s,
                 int recv_timeout_ms)
{
    c->sock = -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(porta);
    addr.sin_addr.s_addr = inet_addr(broker_ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    if (recv_timeout_ms > 0) {
        struct timeval tv;
        tv.tv_sec  = recv_timeout_ms / 1000;
        tv.tv_usec = (recv_timeout_ms % 1000) * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    /* ── Monta CONNECT (MQTT 3.1.1) ── */
    uint8_t body[128];
    int b = 0;
    body[b++] = 0x00; body[b++] = 0x04;            /* Protocol Name length */
    body[b++] = 'M'; body[b++] = 'Q'; body[b++] = 'T'; body[b++] = 'T';
    body[b++] = 0x04;                              /* Protocol Level (4) */
    body[b++] = 0x02;                              /* Flags: Clean Session */
    body[b++] = (uint8_t)(keepalive_s >> 8);
    body[b++] = (uint8_t)(keepalive_s & 0xFF);

    int idlen = (int)strlen(client_id);
    if (idlen > (int)sizeof(body) - b - 2) idlen = (int)sizeof(body) - b - 2;
    body[b++] = (uint8_t)(idlen >> 8);
    body[b++] = (uint8_t)(idlen & 0xFF);
    memcpy(body + b, client_id, idlen);
    b += idlen;

    if (send_packet(sock, MQTT_CONNECT, body, b) < 0) {
        close(sock);
        return -1;
    }

    /* ── Aguarda CONNACK ── */
    uint8_t hdr;
    if (recv_header(sock, &hdr) != 1 || (hdr & 0xF0) != MQTT_CONNACK) {
        close(sock);
        return -1;
    }
    int rlen;
    if (recv_remaining_len(sock, &rlen) < 0 || rlen < 2 || rlen > (int)sizeof(body)) {
        close(sock);
        return -1;
    }
    if (recv_all(sock, body, rlen) < 0 || body[1] != 0x00) { /* return code != aceito */
        close(sock);
        return -1;
    }

    c->sock = sock;
    return 0;
}

int mqtt_publish(mqtt_client_t *c, const char *topico, const char *payload)
{
    if (c->sock < 0) return -1;

    uint16_t tlen = (uint16_t)strlen(topico);
    uint16_t plen = (uint16_t)strlen(payload);

    uint8_t body[256];
    if (2 + tlen + plen > (int)sizeof(body)) return -1;

    int b = 0;
    body[b++] = (uint8_t)(tlen >> 8);
    body[b++] = (uint8_t)(tlen & 0xFF);
    memcpy(body + b, topico, tlen);  b += tlen;
    memcpy(body + b, payload, plen); b += plen;

    return send_packet(c->sock, MQTT_PUBLISH, body, b);
}

int mqtt_subscribe(mqtt_client_t *c, const char *topico)
{
    if (c->sock < 0) return -1;

    uint16_t tlen = (uint16_t)strlen(topico);

    uint8_t body[128];
    if (2 + 2 + tlen + 1 > (int)sizeof(body)) return -1;

    int b = 0;
    body[b++] = 0x00; body[b++] = 0x01;            /* Packet Identifier */
    body[b++] = (uint8_t)(tlen >> 8);
    body[b++] = (uint8_t)(tlen & 0xFF);
    memcpy(body + b, topico, tlen); b += tlen;
    body[b++] = 0x00;                              /* QoS solicitado = 0 */

    return send_packet(c->sock, MQTT_SUBSCRIBE, body, b);
}

int mqtt_ping(mqtt_client_t *c)
{
    if (c->sock < 0) return -1;
    return send_packet(c->sock, MQTT_PINGREQ, NULL, 0);
}

int mqtt_poll(mqtt_client_t *c, char *topico, int topico_sz,
              char *payload, int payload_sz)
{
    if (c->sock < 0) return -1;

    uint8_t hdr;
    int h = recv_header(c->sock, &hdr);
    if (h == 0) return 0;     /* timeout */
    if (h < 0)  return -1;    /* desconectado */

    int rlen;
    if (recv_remaining_len(c->sock, &rlen) < 0) return -1;

    uint8_t type = hdr & 0xF0;

    if (type == MQTT_PUBLISH) {
        uint8_t buf[256];
        if (rlen > (int)sizeof(buf)) {                 /* grande demais: descarta */
            return drain(c->sock, rlen) < 0 ? -1 : 0;
        }
        if (rlen > 0 && recv_all(c->sock, buf, rlen) < 0) return -1;

        int pos = 0;
        uint16_t tlen = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
        pos += 2;
        if (pos + tlen > rlen) return 0;               /* malformado: ignora */

        int tcpy = tlen < (uint16_t)(topico_sz - 1) ? tlen : (topico_sz - 1);
        memcpy(topico, buf + pos, tcpy);
        topico[tcpy] = '\0';
        pos += tlen;
        /* QoS 0: sem Packet Identifier. */

        int plen = rlen - pos;
        if (plen < 0) plen = 0;
        int pcpy = plen < (payload_sz - 1) ? plen : (payload_sz - 1);
        memcpy(payload, buf + pos, pcpy);
        payload[pcpy] = '\0';
        return 1;
    }

    /* SUBACK, PINGRESP ou outros: consome o corpo e ignora. */
    if (rlen > 0 && drain(c->sock, rlen) < 0) return -1;
    return 0;
}

void mqtt_close(mqtt_client_t *c)
{
    if (c->sock < 0) return;
    send_packet(c->sock, MQTT_DISCONNECT, NULL, 0);
    close(c->sock);
    c->sock = -1;
}
