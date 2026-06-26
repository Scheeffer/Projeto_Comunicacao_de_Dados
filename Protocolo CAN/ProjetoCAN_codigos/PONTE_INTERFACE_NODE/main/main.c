#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_http_client.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "mcp2515.h"
#include "can.h"

#include "interface.h"

#define TAG "ESP32_CAN_RECEIVER"

// ======================================================
// WIFI
// ======================================================
#define WIFI_SSID "COM_N_26.1"
#define WIFI_PASS "12345678" 

static EventGroupHandle_t wifi_event_group;

// ======================================================
// NODE-RED SERVER
// ======================================================
#define NODE_RED_IP   "192.168.0.50" 
#define NODE_RED_URL  "http://" NODE_RED_IP ":1880/can"

// ======================================================
// MCP2515 SPI PINOUT
// ======================================================
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// ======================================================
// VARIÁVEIS GLOBAIS 
// ======================================================
uint16_t g_valor_can_bruto = 0;
float g_velocidade = 0.0;
int g_slider_value = 0;        
int g_node_red_slider = 0;     
int g_node_red_freq = 0;       
bool g_modo_remoto = false;    

// ======================================================
// SPI INIT
// ======================================================
bool SPI_Init(void)
{
    spi_bus_config_t bus_cfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 10000000,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 10
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &MCP2515_Object->spi));

    return true;
}

// ======================================================
// HTTP CLIENT - ENVIO NODE-RED
// ======================================================
void send_data_to_nodered(uint16_t valor_bruto, float velocidad)
{
    char json_data[150];
    snprintf(json_data, sizeof(json_data), "{\"valor_can\":%u,\"velocidade\":%.1f}", valor_bruto, velocidad);

    esp_http_client_config_t config = {
        .url = NODE_RED_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 500,
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) esp_wifi_connect();
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) xEventGroupSetBits(wifi_event_group, BIT0);
}

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS } };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    xEventGroupWaitBits(wifi_event_group, BIT0, false, true, pdMS_TO_TICKS(4000));
}

// ======================================================
// WEB SERVER HANDLERS
// ======================================================
esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* html_page = generate_html_page("OFF", "OK", (int)g_valor_can_bruto, g_velocidade, g_slider_value, g_node_red_freq);
    if (html_page != NULL) {
        httpd_resp_send(req, html_page, strlen(html_page));
        free((void*)html_page);
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
}

esp_err_t data_get_handler(httpd_req_t *req)
{
    char json[250]; 
    snprintf(json, sizeof(json),
        "{\"valor_can\":%u,\"velocidade\":%.1f,\"slider_local\":%d,\"nr_freq\":%d,\"modo_remoto\":%s}", 
        g_valor_can_bruto, g_velocidade, g_slider_value, g_node_red_freq, g_modo_remoto ? "true" : "false"
    );
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t slider_post_handler(httpd_req_t *req)
{
    char buf[20];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    g_slider_value = atoi(buf);

    char slider_url[100];
    snprintf(slider_url, sizeof(slider_url), "http://%s:1880/slider", NODE_RED_IP);

    esp_http_client_config_t config = { .url = slider_url, .method = HTTP_METHOD_POST, .timeout_ms = 500 };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client != NULL) {
        esp_http_client_set_header(client, "Content-Type", "text/plain");
        esp_http_client_set_post_field(client, buf, strlen(buf));
        esp_http_client_perform(client);
        esp_http_client_cleanup(client);
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t toggle_post_handler(httpd_req_t *req)
{
    static bool estado_botao = false;
    estado_botao = !estado_botao; 

    const char* resp = estado_botao ? "true" : "false";
    char toggle_url[100];
    snprintf(toggle_url, sizeof(toggle_url), "http://%s:1880/toggle", NODE_RED_IP);

    esp_http_client_config_t config = { .url = toggle_url, .method = HTTP_METHOD_POST, .timeout_ms = 500 };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client != NULL) {
        esp_http_client_set_header(client, "Content-Type", "text/plain");
        esp_http_client_set_post_field(client, resp, strlen(resp));
        esp_http_client_perform(client);
        esp_http_client_cleanup(client);
    }
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

esp_err_t mode_post_handler(httpd_req_t *req)
{
    g_modo_remoto = !g_modo_remoto; 
    struct can_frame frame_cmd;
    frame_cmd.can_id = 0x100; 
    frame_cmd.can_dlc = 2;
    frame_cmd.data[0] = 1; // Injeta flag de dados ativa
    frame_cmd.data[1] = (uint8_t)g_node_red_slider;

    MCP2515_sendMessageAfterCtrlCheck(&frame_cmd); 

    const char* resp = g_modo_remoto ? "true" : "false";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

// Rota de Injeção Direta vinda do Node-RED
esp_err_t nodered_value_post_handler(httpd_req_t *req)
{
    char buf[20];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    g_node_red_slider = atoi(buf);
    ESP_LOGI(TAG, "[CONCORRÊNCIA] Encaminhando Node-RED para a rede CAN: %d", g_node_red_slider);

    // Envia direto no barramento. Quem ler na placa CANA assume o valor.
    struct can_frame frame_cmd;
    frame_cmd.can_id = 0x100;
    frame_cmd.can_dlc = 2;
    frame_cmd.data[0] = 1; 
    frame_cmd.data[1] = (uint8_t)g_node_red_slider;
    
    MCP2515_sendMessageAfterCtrlCheck(&frame_cmd);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

esp_err_t nodered_freq_post_handler(httpd_req_t *req)
{
    char buf[20];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    g_node_red_freq = atoi(buf);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);

    httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
    httpd_register_uri_handler(server, &root);
    httpd_uri_t data = { .uri = "/data", .method = HTTP_GET, .handler = data_get_handler };
    httpd_register_uri_handler(server, &data);
    httpd_uri_t slider = { .uri = "/set_slider", .method = HTTP_POST, .handler = slider_post_handler };
    httpd_register_uri_handler(server, &slider);
    httpd_uri_t toggle = { .uri = "/toggle", .method = HTTP_POST, .handler = toggle_post_handler };
    httpd_register_uri_handler(server, &toggle);
    httpd_uri_t nr_value = { .uri = "/set_nodered_value", .method = HTTP_POST, .handler = nodered_value_post_handler };
    httpd_register_uri_handler(server, &nr_value);
    httpd_uri_t set_mode = { .uri = "/set_mode", .method = HTTP_POST, .handler = mode_post_handler };
    httpd_register_uri_handler(server, &set_mode);
    httpd_uri_t nr_freq = { .uri = "/set_nodered_freq", .method = HTTP_POST, .handler = nodered_freq_post_handler };
    httpd_register_uri_handler(server, &nr_freq);
}

// ======================================================
// CAN TASK RECEPTORA (Monitora a Saída Real unificada no ID 0x4D2)
// ======================================================
void CAN_Task(void *pvParameters)
{
    struct can_frame frame_geral; 

    while (1)
    {
        if (MCP2515_readMessageAfterStatCheck(&frame_geral) == ERROR_OK)
        {
            if (frame_geral.can_id == 0x4D2)
            {
                g_valor_can_bruto = frame_geral.data[0] | (frame_geral.data[1] << 8);
                g_velocidade = (float)g_valor_can_bruto / 10.0f;

                send_data_to_nodered(g_valor_can_bruto, g_velocidade);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    printf("\n====================================\n");
    printf("   CANB: GATEWAY e Interface HTML  \n");
    printf("====================================\n");

    ESP_ERROR_CHECK(nvs_flash_init());
    MCP2515_init();
    SPI_Init();
    MCP2515_reset();
    vTaskDelay(pdMS_TO_TICKS(50));
    MCP2515_setBitrate(CAN_250KBPS, MCP_8MHZ);
    MCP2515_setNormalMode();

    wifi_init();
    start_webserver();

    xTaskCreate(CAN_Task, "CAN_Task", 4096, NULL, 5, NULL);
}