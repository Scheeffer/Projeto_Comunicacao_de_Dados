# 🌐 Plano de endereçamento IP

Use IPs **fixos** (estáticos ou reserva por MAC) — IP dinâmico quebra a integração no meio do teste.

## Sub-rede

```
Rede:     192.168.0.0/24
```

## Dispositivos (parcialmente confirmados pelos flows)

| Dispositivo | Dupla | IP | Protocolo p/ backbone | Observação |
|-------------|:-----:|----|------------------------|------------|
| **CLP S7-121x C** | 1 | `192.168.0.1` | S7 / ISO-on-TCP (rack 0, slot 1) | DB4: START/STOP/freq |
| **ESP32 CAN** | 2 | `192.168.0.63` | HTTP REST (`/set_nodered_freq`, `/set_nodered_value`) | Servidor HTTP no ESP |
| Notebook (Node-RED) | 1 | `192.168.0.___` | host do hub | **confirmar IP** (dashboard :1880) |
| Broker MQTT (Mosquitto) | — | `192.168.0.___` | :1883 | **a definir** (pode ser o mesmo host) |
| ESP32 MQTT | 3 | `192.168.0.___` | MQTT | **definir IP**; apontar ao broker |

> ⚠️ **Conflito de IP a resolver:** o CLP está em `192.168.0.1`, que costuma ser o **gateway** padrão de roteadores. Se houver roteador na rede, ou troque o IP do CLP, ou garanta que não há gateway em `.1`.

## Checklist da Fase de Integração

- [ ] Definir e fixar o IP do **notebook Node-RED** e do **broker MQTT**
- [ ] ESP32 MQTT apontando para o IP do broker (não `localhost`)
- [ ] Firewall do notebook liberando portas 1880 (dashboard) e 1883 (broker)
- [ ] Testar `ping` entre todos os nós antes do teste final
- [ ] Nomes de dispositivos PROFINET configurados no TIA Portal
