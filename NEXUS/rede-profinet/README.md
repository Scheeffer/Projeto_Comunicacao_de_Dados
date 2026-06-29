# 🟦 Rede PROFINET — Célula 1 (Cainã & Matheus)

[![Protocolo](https://img.shields.io/badge/protocolo-PROFINET-blue.svg)](https://www.profibus.com/)
[![CLP](https://img.shields.io/badge/CLP-Siemens%20S7--1217C-teal.svg)](#)

> 🧩 **Template** — preencher com as informações da Dupla 1. As partes conhecidas já estão pré‑preenchidas.

---

## 1. Descrição do projeto

Célula que usa **PROFINET** (Ethernet industrial, baseado em Ethernet/IP padrão) como protocolo local, com um CLP Siemens como mestre. O CLP atua também como **bridge** para o backbone — PROFINET já é Ethernet, então o "subir para o nível Ethernet" se dá expondo as variáveis via **OPC UA** (servidor nativo do S7‑1200, firmware ≥ 4.4) ou **MQTT** (biblioteca LMQTT/`MqttClient` no TIA Portal).

| Item | Valor |
|------|-------|
| Controlador | CLP Siemens **S7‑1217 C** (endpoint `192.168.0.1`, rack 0 / slot 1) |
| Sensor / IHM | **IHM KTP700 Basic** (endpoint `192.168.0.10`) |
| Atuador | **Inversor de frequência SINAMICS G120C** (endpoint `192.168.0.5`) |
| Bridge backbone | **S7 / ISO‑on‑TCP** via `node‑red‑contrib‑s7` (cycletime 1000 ms) |
| Ambiente | TIA Portal _(versão: V20)_ |

### Variáveis expostas ao Node-RED (DB4)

| Nome | Endereço | Tipo | Uso |
|------|----------|------|-----|
| `START` | `DB4.DBX0.0` | bool | Liga o inversor |
| `STOP` | `DB4.DBX0.1` | bool | Desliga o inversor |
| `ENTRADA_REF_FREQUENCIA` | `DB4,REAL2` | real | Seta frequência |
| `FDK_HZ` | `DB2,REAL6` | real | Feedback de frequência |
| `RESET_INV` | `DB4,x0.2` | bool | Reseta as falhas no inversor de frequência |
| `HABILITA NODE RED` | `DB2,X10.1` | bool | Habilita o comando via node red |
| `FDK_VEL` | `DB6,REAL4` | real | Feedback da velocidade rede can |
| `SET_VEL` | `DB6,REAL0` | real | Seta o valor da velocidade rede can |
| `Liga_AQ` | `DB7,X0.0` | bool | Liga o Aquecedor rede MQTT |
| `Liga_Vent` | `DB7,X0.1` | bool | Liga o Ventilador rede MQTT |
| `Desliga_Vent_AQ` | `DB7,X0.2` | bool | Desliga o Ventilador ou o Aquecedor rede MQTT |
| `FDK_temp` | `DB7,REAL2` | real | Feedback do valor da temperatura rede MQTT |


---

## 2. Diagrama de blocos

```mermaid
flowchart LR
    IHM["IHM KTP700 Basic"] --- PLC["CLP S7-121x C<br/>192.168.0.1 (DB4)"]
    G120["Inversor G120C"] --- PLC
    PLC == "S7 / ISO-on-TCP" ==> NR["Node-RED (backbone)"]
```

> ⚠️ **Nota técnica:** o documento original do grupo afirma que esta célula "utiliza o protocolo MQTT". Isso é um erro de copiar‑colar — o protocolo **local** é **PROFINET**, e a ponte ao Node-RED é via **protocolo S7 (ISO-on-TCP)**, não MQTT.

---

## 3. Sequência / estados (preencher)

```mermaid
stateDiagram-v2
    [*] --> Parado
    Parado --> Acionando: comando START
    Acionando --> Rodando: rampa de aceleração
    Rodando --> Parado: comando STOP
    Rodando --> Falha: alarme do inversor
    Falha --> Parado: reconhecer falha
```

---

## 4. Componentes e versões

Ver [`componentes/README.md`](componentes/README.md). Tabela de versões em construção (TIA Portal, firmware do CLP, GSDML do G120C).

---

## 5. Conteúdo desta pasta

```text
rede-profinet/
├── README.md
├── projeto-tia/   ← projeto TIA Portal exportado (.zap / .ap)
├── diagramas/     ← blocos, ladder/FBD, rede PROFINET
├── componentes/   ← S7-1214C, KTP700, G120C (datasheets/links)
└── figs/          ← fotos da bancada, telas da IHM
```
