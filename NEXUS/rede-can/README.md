# 🟥 Rede CAN — Célula 2 (Alexandre & Alvaro)

[![Protocolo](https://img.shields.io/badge/protocolo-CAN%202.0A-red.svg)](https://www.iso.org/standard/63648.html)
[![Controlador](https://img.shields.io/badge/Gateway-ESP32%20%2F%20MCP2515-orange.svg)](#)

---

## 1. Descrição do projeto

O protocolo local utilizado nesta célula é o **CAN (Controller Area Network)** operando a uma taxa de barramento industrial de **250 Kbps**. A rede é composta por microcontroladores **ESP32** acoplados a controladores autônomos de protocolo **MCP2515** via interface de periféricos serial (**SPI**). O ESP32 principal atua como o nó mestre/gateway local da bancada, coletando os sinais do barramento e disponibilizando uma interface gráfica de monitoramento por meio de um Web Server HTTP nativo. 

O grande objetivo desta célula é ler de maneira contínua os dados de um sensor analógico (potenciômetro) mapeado sob o identificador exclusivo CAN `0x4D2`, processar os pacotes para o cálculo de velocidade real em km/h e comandar um painel atuador de indicadores (Painel E620) via ID CAN `0x100`. O Gateway ESP32 também atua como **bridge** para o backbone (Node-RED) por meio de requisições assíncronas **HTTP (POST/GET)** em formato de texto puro (`text/plain`) e JSON. A grande vantagem desse design é garantir a operação offline e robusta da rede de campo CAN, enquanto permite a convergência com o sistema supervisório centralizado.

| Item | Valor |
|------|-------|
| Controlador Base | **Microcontrolador ESP32** (NodeMCU - Camada de Aplicação) |
| Controlador CAN | **Módulo MCP2515** + Transceptor TJA1050 (Cristal de 8MHz / SPI) |
| Atuador Local | **Painel de Indicadores de Bancada E620** (Alvo no ID `0x100`) |
| Bridge backbone | **HTTP Client (POST / GET)** nativo via `esp_http_client` (MIME: `text/plain`) |
| Software | VS Code + ESP-IDF (Espressif IoT Development Framework) |

### Variáveis Disponíveis ao Node-RED / Servidor HTTP

| Nome | Rota / Endpoint | Tipo | Uso |
|------|----------------|------|-----|
| `g_valor_can_bruto` | `/data` (JSON) | uint16_t | Valor decimal bruto do potenciômetro lido no ID CAN `0x4D2` |
| `g_velocidade` | `/data` (JSON) | real | Velocidade física calculada em km/h (`g_valor_can_bruto / 10.0f`) |
| `g_slider_value` | `/set_slider` | int | Frequência de ajuste local definida pela página HTML do ESP32 |
| `g_node_red_slider` | `/set_nodered_value` | int | Injeção de setpoint vinda do Node-RED para a rede CAN (ID `0x100`) |
| `g_node_red_freq` | `/set_nodered_freq` | int | Referência de frequência lida do CLP e repassada ao ESP32 |
| `ligar` | `/ligar` (POST) | bool | comando para ligar o clp |
| `desligar` | `/desligar` (POST) | bool | comnado para desligar o clp |

---

## 2. Diagrama de blocos

```mermaid
graph TD
    %% Linhas do Barramento Principal
    subgraph Barramento_CAN [Barramento Físico CAN - 250 Kbps]
        CANH[Linha CANH - Fio Vermelho]
        CANL[Linha CANL - Fio Verde]
    end

    %% Nó Transmissor (Sensor)
    subgraph No_Sensor [Nó 1: Sensor / Transmissor]
        POT[Potenciômetro Linear] -->|Sinal Analógico| ESP_S[ESP32 Nó Sensor]
        ESP_S <-->|Interface SPI| MCP_S[Transceiver MCP2515]
    end

    %% Nó Atuador Físico
    subgraph No_Atuador [Nó 2: Atuador Local]
        PAINEL[Painel de Indicadores E620]
    end

    %% Nó Gateway (Conexão Backbone)
    subgraph No_Gateway [Nó 3: Gateway / Receptor]
        MCP_G[Transceiver MCP2515] <-->|Interface SPI| ESP_G[ESP32 Gateway Central]
        ESP_G <-->|Wi-Fi: HTTP POST/GET| NR["Node-RED (backbone)<br/>192.168.0.11"]
    end

    %% Conexões ao barramento físico
    MCP_S <--> CANH
    MCP_S <--> CANL
    
    PAINEL <--> CANH
    PAINEL <--> CANL
    
    MCP_G <--> CANH
    MCP_G <--> CANL

    %% Estilização de Cores
    classDef canh_style fill:#ef4444,stroke:#333,stroke-width:2px,color:#fff;
    classDef canl_style fill:#22c55e,stroke:#333,stroke-width:2px,color:#fff;
    classDef esp_style fill:#1e293b,stroke:#0284c7,stroke-width:2px,color:#fff;
    classDef dev_style fill:#334155,stroke:#475569,stroke-width:1px,color:#fff;
    
    class CANH canh_style;
    class CANL canl_style;
    class ESP_S,ESP_G esp_style;
    class POT,PAINEL,NR dev_style;
