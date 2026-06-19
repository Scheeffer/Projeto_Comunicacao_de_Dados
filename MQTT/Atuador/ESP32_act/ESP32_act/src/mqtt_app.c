#include "mqtt_app.h"

#include "mqtt_client.h"

#include "atuador.h"

#include "esp_log.h"

#define TOPICO_PUB "ESP32/COM/Status"
#define TOPICO_SUB "ESP32/COM/Atuador"

estado_sistema_t estado_atual;
acionamento_sistema_t atuador_atual;

static const char *TAG = "MQTT";
char buffer[13] = "STATUS_ATUAL";
static esp_mqtt_client_handle_t cliente;
/******************enderço do broker*****************/
esp_mqtt_client_config_t config =
{
   .broker.address.uri =
       "mqtt://broker.hivemq.com:1883"
};
/*************************************************/
/*
    Callback de eventos MQTT
*/
static void mqtt_event_handler(
        void *handler_args,
        esp_event_base_t base,
        int32_t event_id,
        void *event_data)
{
    esp_mqtt_event_handle_t event =
            (esp_mqtt_event_handle_t)event_data;

    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:

            ESP_LOGI("MQTT",
                     "Broker conectado");

            esp_mqtt_client_subscribe(
                    cliente,
                    TOPICO_SUB,
                    1);

            esp_mqtt_client_publish(
                    cliente,
                    TOPICO_PUB,
                    "Atuador online",
                    0,
                    1,
                    0);

        break;

        case MQTT_EVENT_SUBSCRIBED:

            ESP_LOGI("MQTT","Inscrito");

        break;

        case MQTT_EVENT_DATA:

    char comando[32];

    /* Copia o payload recebido */
    memcpy(comando, event->data, event->data_len);

    /* Adiciona terminador de string */
    comando[event->data_len] = '\0';

    printf("Comando recebido: %s\n", comando);

    if(strcmp(comando, "SYSTEM_OFF") == 0)
    {
        estado_atual = SISTEMA_DESLIGADO;
        atualiza_saidas(DESLIGADO);
    }
    else if(strcmp(comando, "AQUECIMENTO_ON") == 0)
    {
        estado_atual = SISTEMA_AQUECENDO;
        atualiza_saidas(AQUECENDO);
    }
    else if(strcmp(comando, "REFRIGERACAO_ON") == 0)
    {
        estado_atual = SISTEMA_REFRIGERANDO;
        atualiza_saidas(REFRIGERANDO);
    }
    else
    {
        printf("Comando desconhecido\n");
    }

    break;
      

        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGI("MQTT","Desconectado");

        break;
    }
}

void mqtt_start(void)
{
    esp_mqtt_client_config_t config =
    {
        .broker.address.uri =
            "mqtt://broker.hivemq.com:1883"
    };

    cliente =
        esp_mqtt_client_init(&config);

    esp_mqtt_client_register_event(
            cliente,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL);

    esp_mqtt_client_start(cliente);
}
void mqtt_publish_status(void)
{
    if(estado_atual == SISTEMA_DESLIGADO)
    {esp_mqtt_client_publish(cliente,TOPICO_PUB,"Sistema desligado",0,1,0);}
    else if (estado_atual == SISTEMA_AQUECENDO)
    {esp_mqtt_client_publish(cliente,TOPICO_PUB,"Sistema aquecendo",0,1,0);}
    else if (estado_atual == SISTEMA_REFRIGERANDO)
    {esp_mqtt_client_publish(cliente,TOPICO_PUB,"Sistema resfriando",0,1,0);}
    else{
      esp_mqtt_client_publish(cliente,TOPICO_PUB,"Aguardando comando",0,1,0);  
    }
    
}