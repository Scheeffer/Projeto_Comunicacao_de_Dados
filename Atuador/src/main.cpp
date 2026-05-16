#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ===================== CONFIGURAÇÕES =====================
const char* SSID       = "SUA_REDE_WIFI";
const char* WIFI_PASS  = "SUA_SENHA_WIFI";
const char* BROKER_IP  = "192.168.1.100"; // IP do ESP32S3 (Broker) - ajuste conforme sua rede
const int   BROKER_PORT = 1883;

const char* CLIENT_ID     = "ESP8266_Atuador";
const char* TOPIC_ATUADOR = "atuador/comando";
const char* TOPIC_STATUS  = "status/atuador";

// Pino do atuador (relé, LED, motor, etc.)
const int ATUADOR_PIN = 2; // GPIO2 (D4 no NodeMCU)
// =========================================================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

void acionarAtuador(bool ligar) {
    digitalWrite(ATUADOR_PIN, ligar ? HIGH : LOW);
    const char* estadoStr = ligar ? "LIGADO" : "DESLIGADO";
    mqttClient.publish(TOPIC_STATUS, estadoStr);
    Serial.printf("Atuador: %s\n", estadoStr);
}

// Callback chamado ao receber mensagem em tópico assinado
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
    String mensagem;
    for (unsigned int i = 0; i < length; i++) {
        mensagem += (char)payload[i];
    }
    Serial.printf("[RECEBIDO] Tópico: %s | Comando: %s\n", topic, mensagem.c_str());

    if (String(topic) == TOPIC_ATUADOR) {
        acionarAtuador(mensagem == "1");
    }
}

void conectarWiFi() {
    Serial.printf("\nConectando ao WiFi: %s", SSID);
    WiFi.begin(SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi conectado. IP: %s\n", WiFi.localIP().toString().c_str());
}

void conectarMQTT() {
    while (!mqttClient.connected()) {
        Serial.printf("Conectando ao Broker MQTT [%s]...\n", BROKER_IP);
        if (mqttClient.connect(CLIENT_ID)) {
            mqttClient.subscribe(TOPIC_ATUADOR);
            Serial.printf("Broker conectado! Assinando: %s\n", TOPIC_ATUADOR);
        } else {
            Serial.printf("Falha (estado=%d). Nova tentativa em 2s\n", mqttClient.state());
            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ATUADOR_PIN, OUTPUT);
    digitalWrite(ATUADOR_PIN, LOW);

    conectarWiFi();
    mqttClient.setServer(BROKER_IP, BROKER_PORT);
    mqttClient.setCallback(callbackMQTT);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        conectarWiFi();
    }
    if (!mqttClient.connected()) {
        conectarMQTT();
    }
    mqttClient.loop();
}
