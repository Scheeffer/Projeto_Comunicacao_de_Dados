#include "wifi.h"
#include "nvs_flash.h"

/*-----------------------------------------------------------
  EVENTOS DO WIFI
-----------------------------------------------------------*/
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    /* Wi-Fi iniciado */
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Conectando ao Wi-Fi...");
        esp_wifi_connect();
    }

    /* Wi-Fi desconectado */
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Wi-Fi desconectado, tentando reconectar...");
        esp_wifi_connect();
    }

    /* IP recebido */
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *) event_data;

        ESP_LOGI(TAG, "Wi-Fi conectado!");
        ESP_LOGI(TAG, "IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));
    }
}

/*-----------------------------------------------------------
  INICIALIZA WIFI
-----------------------------------------------------------*/
 void wifi_init_sta(void)
{
    esp_err_t ret;

    /* Inicializa NVS */
    ret = nvs_flash_init();

    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) ||
        (ret == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase());

        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    /* Inicializa pilha TCP/IP */
    ESP_ERROR_CHECK(
        esp_netif_init());

    /* Loop de eventos */
    ESP_ERROR_CHECK(
        esp_event_loop_create_default());

    /* Interface station */
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg));

    /* Eventos WiFi */
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            NULL));

    wifi_config_t wifi_config =
    {
        .sta =
        {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        }
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config));

    ESP_ERROR_CHECK(
        esp_wifi_start());

    ESP_LOGI(
        TAG,
        "wifi_init_sta finalizado");
}
