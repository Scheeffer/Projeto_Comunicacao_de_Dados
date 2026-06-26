#ifndef MQTT_APP_H
#define MQTT_APP_H

#include <stdint.h>
typedef enum
{
    SISTEMA_DESLIGADO = 0,
    SISTEMA_AQUECENDO,
    SISTEMA_REFRIGERANDO
} estado_sistema_t;


void mqtt_start(void);
void mqtt_publish_status(void);

#endif