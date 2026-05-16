#include <Arduino.h>
#include <WiFi.h>
#include <uMQTTBroker.h>

// ===================== CONFIGURAÇÕES =====================
const char* SSID      = "SUA_REDE_WIFI";
const char* WIFI_PASS = "SUA_SENHA_WIFI";

const char* TOPIC_SENSOR  = "sensor/dados";
const char* TOPIC_ATUADOR = "atuador/comando";
const char* TOPIC_STATUS  = "status/atuador";

// Limiar: se tensão do sensor > este valor, aciona o atuador
const float LIMIAR = 2.5f;
// =========================================================

// Subclasse do Broker para tratar eventos MQTT
class BrokerMQTT : public uMQTTBroker {
public:
    bool onConnect(IPAddress addr, uint16_t clientCount) override {
        Serial.printf("Cliente conectado: %s (total: %d)\n",
                      addr.toString().c_str(), clientCount);
        return true; // aceitar conexão
    }

    void onDisconnect(IPAddress addr, String clientID) override {
        Serial.printf("Cliente desconectado: %s [%s]\n",
                      addr.toString().c_str(), clientID.c_str());
    }

    // Chamado sempre que qualquer cliente publica em qualquer tópico
    void onData(String topic, const char* data, uint32_t length) override {
        String payload(data, length);
        Serial.printf("[RECEBIDO] Tópico: %s | Payload: %s\n",
                      topic.c_str(), payload.c_str());

        if (topic == TOPIC_SENSOR) {
            processarDadoSensor(payload);
        } else if (topic == TOPIC_STATUS) {
            Serial.printf("[STATUS ATUADOR] %s\n", payload.c_str());
        }
    }

private:
    void processarDadoSensor(const String& payload) {
        float valor = payload.toFloat();
        Serial.printf("Valor do sensor: %.2f V (limiar: %.2f V)\n", valor, LIMIAR);

        const char* comando = (valor > LIMIAR) ? "1" : "0";
        publish(TOPIC_ATUADOR, comando);
        Serial.printf("[ENVIADO] Tópico: %s | Comando: %s\n",
                      TOPIC_ATUADOR, comando);
    }
};

BrokerMQTT broker;

void conectarWiFi() {
    Serial.printf("\nConectando ao WiFi: %s", SSID);
    WiFi.begin(SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi conectado!\nIP do Broker: %s\n",
                  WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(115200);
    conectarWiFi();

    // Iniciar broker na porta padrão MQTT (1883), máximo 10 clientes
    broker.init(1883, 10);
    Serial.println("Broker MQTT iniciado na porta 1883");
}

void loop() {
    // Broker opera em background via callbacks
    // Lógica adicional de controle pode ser inserida aqui
}
