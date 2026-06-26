# 📡 Convenção de tópicos MQTT

Padrão de nomes acordado pela turma. Tópicos mal padronizados são a causa #1 de falha de integração.

## Estrutura

```
d<dupla>/<escopo>/<categoria>/<variavel>
```

| Campo | Valores | Significado |
|-------|---------|-------------|
| `<dupla>` | `d1`, `d2`, `d3` | Identifica a célula de origem |
| `<escopo>` | `local`, `backbone` | `local` = só dentro da célula; `backbone` = visível a todos |
| `<categoria>` | `sensor`, `atuador`, `diag` | Tipo de variável |
| `<variavel>` | livre | Nome curto (ex. `temp`, `cmd`, `online`) |

### Exemplos (Célula 3)

| Tópico | Escopo | Uso |
|--------|--------|-----|
| `d3/local/sensor/temp` | local | Só circula no broker do ESP32‑S3 |
| `d3/local/atuador/cmd` | local | Comando do relé dentro da célula |
| `d3/backbone/sensor/status` | backbone | Espelhado para as outras células |
| `d3/backbone/atuador/cmd` | backbone | Override remoto |
| `d3/backbone/diag/online` | backbone | Bit de presença |

## Níveis de QoS (escolha justificada)

| QoS | Garantia | Quando usar aqui |
|:---:|----------|------------------|
| **0** — *at most once* | Sem confirmação ("fire and forget") | Telemetria periódica (temperatura): se perder uma amostra, a próxima chega logo |
| **1** — *at least once* | Entrega garantida, pode duplicar | **Comandos de atuador** e **diagnóstico** — não pode perder |
| **2** — *exactly once* | Entrega única garantida (handshake duplo) | Em geral desnecessário aqui; maior latência/overhead |

> **Trade‑off:** QoS 0 é mais leve e rápido (bom para microcontroladores), mas não detecta perda. QoS 1 garante o comando, ao custo de tráfego extra e possível duplicação — por isso o receptor deve ser **idempotente** (receber `cmd=1` duas vezes = ligar e continuar ligado).

## `retained` e LWT

- **`retained`**: o broker guarda a última mensagem do tópico e entrega a quem assinar depois. Útil para `diag/online` e setpoints.
- **LWT (Last Will and Testament)**: mensagem que o broker publica **automaticamente** se o cliente cair sem avisar. É como implementamos o bit de diagnóstico: `diag/online = 0` quando o nó some.

---

## ⚠️ Escopo desta convenção

No estado atual do projeto, **apenas a Célula 3 usa MQTT**. A Célula 1 (PROFINET) fala **S7/ISO-on-TCP** com o Node-RED e a Célula 2 (CAN) fala **HTTP REST**. Portanto, esta convenção de tópicos vale principalmente para a Célula 3 e para o(s) flow(s) MQTT do Node-RED. A unificação de todas as variáveis acontece na **Tabela Global de Variáveis** dentro do Node-RED — ver [`tabela-global-variaveis.md`](tabela-global-variaveis.md).
