#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>  // Librería oficial DHT

// ================= CONFIGURACIÓN =====================
const char* WIFI_SSID = "HONORX5b";
const char* WIFI_PASSWORD = "001122336";

const char* MQTT_SERVER = "10.66.76.174";
const int   MQTT_PORT   = 1883;

// Pin y tipo de sensor
#define DHT_PIN 4
#define DHT_TYPE DHT22
// =====================================================

// Inicializar sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Variables globales
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;

// ================= FUNCIONES =========================
void conectarWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32_DHT22")) {
      Serial.println("¡Conectado al Broker!");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5s");
      delay(5000);
    }
  }
}
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=================================");
  Serial.println("  ESP32 - DHT22 MQTT");
  Serial.println("=================================");

  dht.begin(); // Inicializar sensor
  conectarWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long ahora = millis();
  if (ahora - ultimoEnvio > 10000) { // Cada 10 segundos
    ultimoEnvio = ahora;

    // Leer temperatura y humedad
    float humedad = dht.readHumidity();
    float temperatura = dht.readTemperature();

    if (isnan(humedad) || isnan(temperatura)) {
      Serial.println("Error leyendo DHT22");
      return;
    }

    // Crear payload JSON
    String payload = "{";
    payload += "\"temperatura\":" + String(temperatura) + ",";
    payload += "\"humedad\":" + String(humedad) + "}";

    // Publicar en MQTT
    client.publish("casa/ambiente", payload.c_str());
    Serial.println("Publicado en MQTT: " + payload);
  }
}