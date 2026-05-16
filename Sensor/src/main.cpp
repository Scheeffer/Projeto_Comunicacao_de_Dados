#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ===================== CONFIGURAÇÕES =====================
const char* SSID       = "SUA_REDE_WIFI";
const char* WIFI_PASS  = "SUA_SENHA_WIFI";
const char* BROKER_IP  = "192.168.1.100"; // IP do ESP32S3 (Broker) - ajuste conforme sua rede
const int   BROKER_PORT = 1883;

const char* CLIENT_ID    = "ESP8266_Sensor";
const char* TOPIC_SENSOR = "sensor/dados";

const int   SENSOR_PIN        = A0;
const unsigned long PUB_INTERVAL = 5000; // publicar a cada 5 segundos
// =========================================================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long ultimaPublicacao = 0;

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
            Serial.println("Broker MQTT conectado!");
        } else {
            Serial.printf("Falha (estado=%d). Nova tentativa em 2s\n", mqttClient.state());
            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    conectarWiFi();
    mqttClient.setServer(BROKER_IP, BROKER_PORT);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        conectarWiFi();
    }
    if (!mqttClient.connected()) {
        conectarMQTT();
    }
    mqttClient.loop();

    unsigned long agora = millis();
    if (agora - ultimaPublicacao >= PUB_INTERVAL) {
        ultimaPublicacao = agora;

        int   raw     = analogRead(SENSOR_PIN);
        float tensao  = raw * (3.3f / 1023.0f);

        char payload[32];
        snprintf(payload, sizeof(payload), "%.2f", tensao);

        bool ok = mqttClient.publish(TOPIC_SENSOR, payload);
        Serial.printf("Publicado [%s] -> \"%s\" %s\n",
                      TOPIC_SENSOR, payload, ok ? "OK" : "FALHOU");
    }
}
