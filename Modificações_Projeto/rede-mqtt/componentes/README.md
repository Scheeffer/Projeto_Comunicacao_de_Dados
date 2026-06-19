# Componentes — Rede MQTT

| Componente | Especificação | Qtd | Datasheet/Link |
|-----------|---------------|:---:|----------------|
| ESP32 DevKit | `esp32doit-devkit-v1` (ESP-IDF) | 1 | espressif.com |
| Sensor de temperatura | _definir_ (DS18B20? termistor?) | 1 | — |
| Relé/SSR aquecimento | aciona elemento de aquecimento (GPIO18) | 1 | _preencher_ |
| Relé/SSR refrigeração | aciona ventilador/peltier (GPIO19) | 1 | _preencher_ |
| Fonte | alimentação do ESP + cargas | 1 | — |

> Se o sensor for DS18B20: incluir resistor de pull-up 4k7 no barramento 1-Wire.
