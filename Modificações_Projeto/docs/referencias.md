# 🔗 Referências e links úteis

Curadoria inicial — **adicione os links que você for usando**.

## Protocolos

### MQTT
- Site oficial / especificação — https://mqtt.org/
- MQTT 3.1.1 (OASIS) — https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
- HiveMQ MQTT Essentials (didático) — https://www.hivemq.com/mqtt-essentials/
- PubSubClient (lib Arduino) — https://github.com/knolleary/pubsubclient
- AsyncMqttClient — https://github.com/marvinroger/async-mqtt-client
- uMQTTBroker (broker em ESP) — https://github.com/martin-ger/uMQTTBroker

### PROFINET
- PROFIBUS & PROFINET International (PI) — https://www.profibus.com/
- Siemens SIMATIC S7‑1200 — https://support.industry.siemens.com/
- OPC UA no S7‑1200 (servidor nativo) — buscar "S7-1200 OPC UA server" no Siemens Support
- SINAMICS G120C (inversor) — https://support.industry.siemens.com/

### CAN
- Intro didática ao CAN (CSS Electronics) — https://www.csselectronics.com/pages/can-bus-simple-intro-tutorial
- ESP32 TWAI (driver CAN oficial) — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html
- Transceiver SN65HVD230 — https://www.ti.com/product/SN65HVD230

## Infraestrutura / backbone

- Node‑RED — https://nodered.org/  · docs — https://nodered.org/docs/
- node‑red‑dashboard — https://flows.nodered.org/node/node-red-dashboard
- Eclipse Mosquitto — https://mosquitto.org/
- node‑red‑contrib‑s7 (PROFINET/S7 ↔ Node‑RED) — https://flows.nodered.org/node/node-red-contrib-s7

## Ferramentas de desenvolvimento

- PlatformIO — https://platformio.org/  · docs ESP32 — https://docs.platformio.org/en/latest/platforms/espressif32.html
- ESP‑IDF — https://docs.espressif.com/projects/esp-idf/
- esp-mqtt (cliente MQTT do ESP-IDF) — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
- Fork pioarduino (cores ESP novos) — https://github.com/pioarduino/platform-espressif32

## Componentes

- DS18B20 (sensor de temperatura 1‑Wire) — datasheet Analog/Maxim
- DallasTemperature / OneWire (libs) — https://github.com/milesburton/Arduino-Temperature-Control-Library
- ESP32‑S3 — https://www.espressif.com/en/products/socs/esp32-s3
- ESP8266 — https://www.espressif.com/en/products/socs/esp8266

## Versões de referência (jun/2026)

| Software | Versão atual |
|----------|--------------|
| Node‑RED | 5.0 (recomenda Node.js 24.x) |
| Eclipse Mosquitto | 2.1.2 |
| Plataforma `espressif32` (PlatformIO) | 6.x oficial / fork pioarduino p/ cores recentes |
