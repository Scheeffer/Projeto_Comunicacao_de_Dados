# 🟥 Rede CAN — Célula 2 (Alexandre & Alvaro)

[![Protocolo](https://img.shields.io/badge/protocolo-CAN%202.0A-red.svg)](https://www.iso.org/standard/63648.html)
[![Controlador](https://img.shields.io/badge/Gateway-ESP32%20%2F%20MCP2515-orange.svg)](#)

---

## 1. Descrição do projeto

O protocolo local utilizado nesta célula é o **CAN (Controller Area Network)** operando a uma taxa de barramento industrial de **250 Kbps**. A rede é composta por microcontroladores **ESP32** acoplados a controladores autônomos de protocolo **MCP2515** via interface de periféricos serial (**SPI**). O ESP32 principal atua como o nó mestre/gateway local da bancada, coletando os sinais do barramento e disponibilizando uma interface gráfica de monitoramento por meio de um Web Server HTTP nativo. 

O grande objetivo desta célula é ler de maneira contínua os dados de um sensor analógico (potenciômetro) mapeado sob o identificador exclusivo CAN `, processar os pacotes para o cálculo de velocidade real em km/h e comandar um painel atuador de indicadores (Painel E620) via ID CAN `0x4D2`. O Gateway ESP32 também atua como **ponte** para o backbone (Node-RED) por meio de requisições assíncronas **HTTP (POST/GET)** em formato de texto puro (`text/plain`) e JSON. A grande vantagem desse design é garantir a operação offline e robusta da rede de campo CAN, enquanto permite a convergência com o sistema supervisório centralizado.

| Item | Valor |
|------|-------|
| Controladores Base | **Microcontrolador ESP32 WROOM DEV-KIT V1** |
| Controlador CAN | **Módulo MCP2515** + Transceptor TJA1050 (Cristal de 8MHz / SPI) |
| Atuador | **Painel de Indicadores de Bancada E620** ( ID `0x4D2`) |
| Sensor | **Pontenciometro(250kohms) + Microcontrolador ESP32 WROOM DEV-KIT V1** ( ID `0x100`) |
| ponte backbone | **HTTP Client (POST / GET)** nativo via `esp_http_client` (MIME: `text/plain`) |
| Software | ESP-IDF V5.4|

### Variáveis Disponíveis ao Node-RED / Servidor HTTP

| Nome | Rota / Endpoint | Tipo no Node-RED | Uso / Formato de Origem |
|------|----------------|-------------------|-------------------------|
| `g_valor_can_bruto` | `/data` (JSON) | string | Valor decimal bruto do potenciômetro (origem `uint16_t` na CAN) |
| `g_velocidade` | `/data` (JSON) | string | Velocidade física calculada em km/h (origem `float`) |
| `g_slider_value` | `/set_slider` | string | Posição do Slider alterada na página HTML (origem `int`) |
| `g_node_red_slider` | `/set_nodered_value` | string | Setpoint enviado do Node-RED para a rede CAN (convertido para `int` no ESP32) |
| `g_node_red_freq` | `/set_nodered_freq` | string | Referência de frequência do CLP enviada ao ESP32 como texto puro |
| `ligar` | `/ligar` (POST) | string | Comando de partida enviado como o texto `"true"` |
| `desligar` | `/desligar` (POST) | string | Comando de paragem enviado como o texto `"true"` |

---

## 2. Diagrama de blocos

<p align="center">
  <img src="figs/diagrama_rede_can.png" alt="Topologia Física da Rede CAN - Célula 2" width="700">
</p>

---
