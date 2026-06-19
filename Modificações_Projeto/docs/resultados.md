# 📈 Resultados

Métricas medidas no teste de integração. Preencher com dados reais coletados na bancada. Estas medidas dão substância técnica à apresentação e mostram domínio dos conceitos de Comunicação de Dados.

## 1. Latência fim-a-fim

Tempo entre uma ação numa célula e o efeito observado em outra (via backbone).

| Caminho | Medições (ms) | Média | Desvio | Método |
|---------|---------------|:-----:|:------:|--------|
| Sensor C3 → dashboard | _preencher_ | — | — | timestamp pub/sub |
| Dashboard → atuador C1 | _preencher_ | — | — | timestamp cmd/efeito |
| C2 → C3 (cruzado) | _preencher_ | — | — | — |

> **Como medir:** carimbar a hora no publish e no subscribe; a diferença é a latência. No Node‑RED, um nó `function` com `Date.now()` na entrada e saída resolve.

## 2. Perda de pacotes

| Tópico / caminho | Enviados | Recebidos | Perda (%) | QoS |
|------------------|:--------:|:---------:|:---------:|:---:|
| `d3/backbone/sensor/status` | _preencher_ | — | — | 0 |
| `d1/backbone/atuador/cmd` | _preencher_ | — | — | 1 |

> **Interpretação:** com QoS 0 espera‑se alguma perda sob carga; com QoS 1 a perda deve ser ~0% (ao custo de retransmissões). Comparar os dois é um ótimo resultado para discutir.

## 3. Jitter

Variação da latência entre amostras consecutivas (desvio padrão dos intervalos de chegada). Relevante porque telemetria muito irregular prejudica controle.

| Caminho | Jitter (ms) | Observação |
|---------|:-----------:|------------|
| Telemetria C3 | _preencher_ | — |

## 4. Throughput

Vazão útil de mensagens/dados pelo backbone.

| Métrica | Valor | Condição |
|---------|:-----:|----------|
| Mensagens/s (pico) | _preencher_ | todas as células publicando |
| Bytes/s (médio) | _preencher_ | payloads típicos |

## 5. Disponibilidade / diagnóstico

| Célula | Tempo online | Quedas detectadas (LWT) | Recuperação média |
|:------:|:------------:|:-----------------------:|:-----------------:|
| 1 | _preencher_ | — | — |
| 2 | _preencher_ | — | — |
| 3 | _preencher_ | — | — |

---

### Erros comuns na coleta de métricas

- Medir latência sem sincronizar relógios entre dois PCs (use o mesmo host, ou meça round‑trip e divida por 2).
- Confundir **jitter** (variação) com **latência** (valor absoluto).
- Reportar throughput sem dizer a condição de carga.
