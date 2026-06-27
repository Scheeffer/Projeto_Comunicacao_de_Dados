# Componentes — Rede MQTT

| Componente | Especificação | Qtd | Datasheet/Link |
|-----------|---------------|:---:|----------------|
| ESP32 DevKit | `esp32doit-devkit-v1` (ESP-IDF) | 1 | espressif.com |
| Sensor de temperatura + Res. Pull-Up 4k7| DS18B20 | 1 | https://www.makerhero.com/img/files/download/Datasheet-DS18B20.pdf |
| Relé/SSR aquecimento | aciona elemento de aquecimento (GPIO18) | 1 | https://www.makerhero.com/img/files/download/SRD-05VDC-SL-C-Datasheet.pdf |
| Relé/SSR refrigeração | aciona ventilador/peltier (GPIO19) | 1 | https://www.makerhero.com/img/files/download/SRD-05VDC-SL-C-Datasheet.pdf |
| Fonte | alimentação do ESP + cargas | 1 | — |

> Sensor DS18B20: resistor de pull-up 4k7 no barramento 1-Wire.
