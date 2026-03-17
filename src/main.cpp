#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h> // 1. Incluir la librería JSON
#include <secrets.h>

// ================= CONFIGURACIÓN =====================
//const char* WIFI_SSID = "********";
//const char* WIFI_PASSWORD = "*****";

//const char* MQTT_SERVER = "********";
//const int   MQTT_PORT   = ******;

#define DHT_PIN 4
#define DHT_TYPE DHT22
// =====================================================

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
    // ID de cliente único para evitar desconexiones
    if (client.connect("ESP32_DHT22_Client")) { 
      Serial.println("¡Conectado!");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
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

    if (isnan(humedad) || isnan(temperatura)) {
      Serial.println("Error leyendo DHT22");
      return; 
    }

    // --- BLOQUE DE CREACIÓN DE JSON ---
    
    // 2. Crear el documento JSON (capacidad de 128 bytes es suficiente)
    StaticJsonDocument<128> doc;

    // 3. Asignar valores a las llaves
    doc["temp"] = temperatura;
    doc["hum"]  = humedad;
    doc["device"] = "ESP32_DHT22"; // Opcional: Identificar el equipo
    doc["rssi"] = WiFi.RSSI();     // Opcional: Calidad de señal WiFi

    // 4. Convertir el objeto JSON a una cadena de texto (Buffer)
    char buffer[128];
    serializeJson(doc, buffer);

    // 5. Publicar en MQTT usando el buffer generado
    client.publish("casa/ambiental", buffer);

    Serial.print("Publicado JSON: ");
    Serial.println(buffer);
  }
}