# 🧱 Mapeamento OSI dos protocolos

Fundamentação teórica: onde cada protocolo do projeto se encaixa no **modelo OSI**. Isso explica *por que* precisamos de bridges e *como* a "língua geral" (MQTT) coexiste com PROFINET e CAN.

## Visão geral

| Camada OSI | PROFINET (Célula 1) | CAN (Célula 2) | MQTT / Backbone (Célula 3 + integração) |
|:----------:|--------------------|----------------|------------------------------------------|
| 7 Aplicação | Perfis PROFINET (RT/IRT), GSDML | — (aplicação definida pelo usuário) | **MQTT** (publish/subscribe) |
| 6 Apresentação | — | — | Payload (JSON/string/binário) |
| 5 Sessão | Contexto de aplicação (AR/CR) | — | Sessão MQTT (clientId, keepalive) |
| 4 Transporte | UDP (RT) / TCP (aciclico) | — (CAN não tem camada 4) | **TCP** :1883 |
| 3 Rede | IP | — (CAN não tem camada 3) | **IP** |
| 2 Enlace | Ethernet (com priorização VLAN) | **CAN 2.0 (arbitragem por ID, CSMA/CR)** | Ethernet |
| 1 Física | Ethernet 100BASE‑TX (RJ45) | Par diferencial CAN_H/CAN_L + transceiver | Ethernet 100BASE‑TX |

## Pontos teóricos importantes

- **CAN é um protocolo de camadas 1‑2 apenas.** Ele não tem IP nem TCP — por isso *não consegue* falar com o backbone Ethernet diretamente. O ESP32 funciona como **gateway**, "subindo" o dado de CAN (enlace) para MQTT/TCP/IP (transporte/rede/aplicação). É exatamente o que o enunciado pede para redes não‑Ethernet.

- **PROFINET roda sobre Ethernet padrão**, então já está no nível físico do backbone. O desafio não é físico, é de **camada de aplicação**: o CLP precisa expor as variáveis numa linguagem que o Node‑RED entenda (MQTT ou OPC UA).

- **MQTT é leve e assíncrono** (publish/subscribe desacoplado por um broker), o que o torna ideal como "língua geral": cada célula publica sem saber quem consome. Compare com um modelo **request/response** (HTTP): MQTT mantém conexões persistentes e empurra dados (push), reduzindo latência e overhead em telemetria contínua.

- **Arbitragem CAN (CSMA/CR):** diferente de Ethernet (CSMA/CD), o CAN resolve colisões por **prioridade de identificador** sem destruir o frame — o ID menor vence o barramento. Bom gancho teórico para a discussão de controle de acesso ao meio na apresentação.
