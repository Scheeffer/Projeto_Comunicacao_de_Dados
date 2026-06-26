# Node-RED — flows do backbone

Coloque aqui o **flow exportado** do Node-RED.

## Exportar
No editor Node-RED: menu (☰) > Export > All flows > Download → salve como `flows.json` nesta pasta.

## Importar
menu (☰) > Import > selecione `flows.json`.

## Conteúdo esperado
- Conexão ao broker MQTT central (mqtt-broker config node)
- Subscribers dos tópicos `d1/`, `d2/`, `d3/` backbone
- Dashboard (gauges de sensores, botões de atuadores, LEDs de diagnóstico)
- Tabela Global de Variáveis (flow context ou nós function)

> Não comite credenciais. Se o flow tiver senha do broker, remova antes do commit.
