# 🗂️ Tabela Global de Variáveis

A "língua geral" do sistema. Toda célula expõe, no mínimo, **status do sensor**, **comando do atuador** e **bit de diagnóstico** (Regra de Ouro da Interoperabilidade). Mantida no **Node‑RED**.

## Convenção de nomes

```
d<dupla>/backbone/<categoria>/<variavel>
```

- `d1` = PROFINET, `d2` = CAN, `d3` = MQTT
- `<categoria>` ∈ { `sensor`, `atuador`, `diag` }

## Tabela

| Variável (tópico) | Dupla | Tipo | Acesso | Descrição |
|-------------------|:-----:|------|--------|-----------|
| `d1/backbone/sensor/status` | 1 | int/float | leitura | Valor do sensor PROFINET |
| `d1/backbone/atuador/cmd` | 1 | bool/int | escrita | Comando do inversor G120C |
| `d1/backbone/diag/online` | 1 | bool | leitura | Célula 1 online? |
| `d2/backbone/sensor/status` | 2 | int/float | leitura | Valor do potenciômetro (nó CAN) |
| `d2/backbone/atuador/cmd` | 2 | bool/int | escrita | Comando do display E620 |
| `d2/backbone/diag/online` | 2 | bool | leitura | Célula 2 online? |
| `d3/backbone/sensor/status` | 3 | float | leitura | Temperatura DS18B20 (°C) |
| `d3/backbone/atuador/cmd` | 3 | bool | escrita | Comando do relé |
| `d3/backbone/diag/online` | 3 | bool | leitura | Célula 3 online? (via LWT) |

> 📌 **A preencher na Fase de Integração:** tipos exatos, faixas, unidades e quem é o "dono" (publisher) de cada variável. Travar isso **antes** do teste final evita retrabalho.
