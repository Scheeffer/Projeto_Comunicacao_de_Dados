#include <stdio.h>
#include <string.h>
#include <stdlib.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc_cal.h"
#include "hal/adc_types.h"

#include "mcp2515.h"
#include "can.h"

// Definição dos pinos físicos do SPI no ESP32
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Canal do ADC para o potenciômetro no pino GPIO32
#define ADC_CHANNEL ADC_CHANNEL_4

CAN_FRAME_t tx_frame __attribute__((aligned(4)));

// Estruturas de controle do ADC do ESP32
adc_oneshot_unit_handle_t adc_handle;
esp_adc_cal_characteristics_t adc_chars;

uint16_t g_velocidade_sistema = 0;           
uint16_t g_ultimo_valor_potenciometro = 9999; // Linha de base exclusiva para cálculo de variação

bool SPI_Init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK,   
        .mode = 0,                    
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7                
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &MCP2515_Object->spi));

    return true;
}

void ADC_Init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, 
        .atten = ADC_ATTEN_DB_12     
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

void CAN_Task(void *pvParameters)
{
    struct can_frame frame_tx; 
    struct can_frame frame_rx; 

    int adc_raw = 0;
    uint16_t valor_velocidade_pot = 0;

    // Variável estática para guardar o valor em que o potenciômetro estava na última leitura
    static uint16_t ultima_leitura_real_pot = 9999; 

    #define SAMPLES 10 //aquisição de 10 amostras para filtrar ruidos              
    #define VARIACAO_MINIMA_PCT 2.5f // Ajustado para 2.5% para dar uma folga extra contra ruídos
    const int threshold_mudanca = (int)((VARIACAO_MINIMA_PCT / 100.0f) * 1000.0f);

    while(1)
    {
        // ------------------------------------------------------
        //Detecção de eventos para determinar qual das redes assume o atuador(no caso quem envair o dado primeiro assume)
        // EVENTO 1: CHEGOU COMANDO DA REDE (NODE-RED ATRAVÉS DO CANB-gateway)
        // ------------------------------------------------------
        if (MCP2515_readMessageAfterStatCheck(&frame_rx) == ERROR_OK)
        {
            if (frame_rx.can_id == 0x100)
            {
                // Node-RED assume o sistema na hora
                g_velocidade_sistema = (uint16_t)frame_rx.data[1] * 10; 
                printf("[EVENTO REDE] Node-RED assumiu o controle. Valor: %u\n", g_velocidade_sistema);
            }
        }

        // ------------------------------------------------------
        // EVENTO 2: LEITURA FÍSICA E ANÁLISE DO POTENCIÔMETRO
        // ------------------------------------------------------
        int adc_sum = 0;
        int leituras_validas = 0;
        for (int i = 0; i < SAMPLES; i++) {
            if (adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw) == ESP_OK) {
                adc_sum += adc_raw;
                leituras_validas++;
            }
            vTaskDelay(pdMS_TO_TICKS(2)); 
        }
        int adc_filtrado = (leituras_validas > 0) ? (adc_sum / leituras_validas) : 0;
        valor_velocidade_pot = (uint16_t)(((float)adc_filtrado / 4095.0f) * 1000.0f);

        // Na primeira execução, alinha o histórico com a leitura atual
        if (ultima_leitura_real_pot == 9999) {
            ultima_leitura_real_pot = valor_velocidade_pot;
            g_velocidade_sistema = valor_velocidade_pot;
        }

        // A variação é calculada comparando o potenciômetro contra ELE MESMO no ciclo passado
        int diferenca_movimento_fisco = abs((int)valor_velocidade_pot - (int)ultima_leitura_real_pot);
        
        // Se o usuário mexeu o potenciômetro além do ruído...
        if (diferenca_movimento_fisco >= threshold_mudanca) {
            // O botão físico assume o controle do sistema
            g_velocidade_sistema = valor_velocidade_pot;
            printf("[EVENTO HARDWARE] Potenciômetro movido de %u para %u. Assumiu!\n", ultima_leitura_real_pot, valor_velocidade_pot);
        }
        // Garante o zero absoluto se o botão for levado ao batente inicial
        else if (valor_velocidade_pot == 0 && g_velocidade_sistema != 0) {
            g_velocidade_sistema = 0;
            printf("[EVENTO HARDWARE] Zerado pelo potenciômetro\n");
        }

        // Atualiza o histórico do hardware com o valor real lido (independente de quem está controlando)
        ultima_leitura_real_pot = valor_velocidade_pot;

        // ------------------------------------------------------
        // TRANSMISSÃO CÍCLICA FIXA - O painel E620 precisa receber os bytes continuamente para manter o valor no display
        // ------------------------------------------------------
        frame_tx.can_id = 0x4D2;
        frame_tx.can_dlc = 8;
        memset(frame_tx.data, 0, 8);
        
        frame_tx.data[0] = g_velocidade_sistema & 0xFF;        
        frame_tx.data[1] = (g_velocidade_sistema >> 8) & 0xFF; 
        frame_tx.data[5] = 0x40; 
        frame_tx.data[7] = 0x00; 

        MCP2515_sendMessageAfterCtrlCheck(&frame_tx);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
void app_main(void)
{
    printf("\n====================================\n");
    printf("   CANA - ATUADOR e SENSOR\n");
    printf("====================================\n");

    if(MCP2515_init() != ERROR_OK) return;
    SPI_Init();
    if(MCP2515_reset() != ERROR_OK) return;

    vTaskDelay(pdMS_TO_TICKS(100));
    MCP2515_setBitrate(CAN_250KBPS, MCP_8MHZ);
    MCP2515_setNormalMode();
    ADC_Init();
    MCP2515_setRegister(MCP_CANINTF, 0x00);

    xTaskCreate(CAN_Task, "CAN_Task", 4096, NULL, 5, NULL);
}