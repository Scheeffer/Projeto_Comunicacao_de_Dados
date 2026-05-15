# Projeto_Comunicacao_de_Dados
Repositório destinado ao projeto da disciplina de Comunicação de dados

1) Objetivo:
O objetivo desta tarefa é a construção coletiva de um sistema de automação
heterogêneo. Cada dupla será responsável por uma "célula de produção" (rede
específica) que deve, obrigatoriamente, trocar dados com as células das outras
duplas. Ao final, qualquer sensor da sala deve ser capaz de ser lido por
qualquer célula, e qualquer atuador pode ser controlado, independentemente
da rede onde esteja.

2) Divisão de Responsabilidades:
Cada dupla irá construir uma "célula de produção" (rede específica), e irá
definir o protocolo da rede. Cada rede deve disponibilizar uma interface para
leitura/controle de todos os sensores/atuadores em todas as redes.

3) Requisitos de Hardware por Célula:
Cada dupla deve montar uma bancada contendo:
   - 1 Sensor (ex: ultrassônico, temperatura, gás, etc).
   - 1 Atuador (ex: Relé ligado a um dispositivo, Inversor de Frequência, Motor, etc).
   - 1 Controlador (CLP, sistema embarcado genérico) que suporte o protocolo designado.

4) O Protocolo de Integração (A "Língua Geral"):
Para que uma rede fale com a outra, a turma deverá definir em conjunto uma
Tabela Global de Variáveis.

   A) Estratégia de Backbone (Espinha Dorsal)
      A integração será feita através de um Backbone Ethernet.
    - Redes seriais, por exemplo, (Modbus RTU / PROFIBUS) deverão usar um Gateway ou um CLP com duas portas para "subir" seus dados para o nível Ethernet.

    - Todas as duplas devem mapear seus dados para um servidor central (pode ser um CLP Mestre ou um Broker MQTT/OPC UA) ou usar comunicação direta (Peer-to-Peer) entre os controladores.

      B) Regra de Ouro da Interoperabilidade
        Cada dupla deve disponibilizar para a rede global:
     - Status do Sensor: (Booleano ou Inteiro).
     - Comando do Atuador: (Variável que pode ser escrita por terceiros).
     - Diagnóstico: (Bit que indica se a rede daquela dupla está online).

6) Dinâmica do Projeto Final:
   A) Fase de Configuração: Cada dupla coloca sua rede local para funcionar (ela consegue controlar o atuador e ler o sensor interno).
   B) Fase de Integração: As duplas negociam endereços IP e nomes de dispositivos.
   C) O Teste Final: O professor irá, em data de apresentação, testar a rede.

7) Critérios de Avaliação:
   - Autonomia da Célula (25%): A rede local da dupla funciona perfeitamente?
   - Visibilidade Global (20%): Os dados da dupla estão acessíveis para as outras redes?
   - Desenvolvimento do backbone (35%): A dupla participou do desenvolvimento e documentação do backbone de integração, e está funcionando? (sua rede deve esta acessível para leitura e controle dos sensores e atuadores para receber ponto nesse critério).
   - Documentação (20%): A rede e o backbone.

8) Data da apresentação do Sistema Operacional:
   - Data: 26 DE JUNHO DE 2026
- [Projeto_final_de_Comunicao_de_Dados.pdf](https://github.com/user-attachments/files/26843268/Projeto_final_de_Comunicao_de_Dados.pdf)
