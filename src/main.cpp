#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "FABLAB RUWI 2.4G";
const char* WIFI_PASSWORD = "100525Ruwi";
const char* MQTT_SERVER = "192.168.18.25";
const int   MQTT_PORT   = 1883;

// Datos de conexión
const char* DEVICE_ID = "1d4c8195-2bd5-4253-a240-8491470f19b5";
const char* API_KEY   = "a07a75cfd59ea01565feda4c0a7bafd9e11a3e722fd41a2aaa0ef3035c388ff7";
const char* TOPIC_TEL = "/agro/00000000-0000-0000-0000-000000000001/1d4c8195-2bd5-4253-a240-8491470f19b5/telemetry";

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado.");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(DEVICE_ID, API_KEY, API_KEY)) { 
      Serial.println("¡Conectado exitosamente!");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  conectarWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long ahora = millis();
  if (ahora - ultimoEnvio > 10000) { 
    ultimoEnvio = ahora;

    float humedad = dht.readHumidity();
    float temperatura = dht.readTemperature();
    // Obtener la fuerza de la señal WiFi (RSSI)
    long rssi = WiFi.RSSI(); 

    if (isnan(humedad) || isnan(temperatura)) {
      Serial.println("Error leyendo DHT22");
      return; 
    }

    StaticJsonDocument<256> doc;
    JsonObject variables = doc.createNestedObject("variables");
    
    variables["temperature"] = temperatura;
    variables["humidity"] = humedad;
    variables["rssi"] = rssi; // Aquí se envía el nivel de señal en dBm

    char buffer[256];
    serializeJson(doc, buffer);

    if (client.publish(TOPIC_TEL, buffer)) {
      Serial.print("Publicado con éxito en: ");
      Serial.println(TOPIC_TEL);
      Serial.print("Payload: ");
      Serial.println(buffer);
    } else {
      Serial.println("Error al publicar mensaje.");
    }
  }
}