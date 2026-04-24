#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "FABLAB RUWI 2.4G";
const char* WIFI_PASSWORD = "100525Ruwi";
const char* MQTT_SERVER = "192.168.18.25";
const int   MQTT_PORT   = 1883;

const char* DEVICE_ID = "1d4c8195-2bd5-4253-a240-8491470f19b5";
const char* API_KEY   = "a07a75cfd59ea01565feda4c0a7bafd9e11a3e722fd41a2aaa0ef3035c388ff7";
const char* TOPIC_TEL = "/agro/00000000-0000-0000-0000-000000000001/1d4c8195-2bd5-4253-a240-8491470f19b5/telemetry";

#define DHT_PIN 4
#define DHT_TYPE DHT22
#define BATTERY_PIN 34  // <-- Pin donde conectas el divisor de voltaje

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;

// Función para leer el voltaje de la batería
float leerVoltajeBateria() {

    // 1. Leer el voltaje real usando tu factor de 3.45
  int rawADC = analogRead(BATTERY_PIN);
  float voltajeLeido = (rawADC / 4095.0) * 3.3 * 3.45;

  // 2. Definir los límites para 2 pilas (2S)
  float voltMax = 8.4;  // Baterías llenas
  float voltMin = 6.4;  // Baterías vacías (punto de corte seguro)

  // 3. Calcular el porcentaje
  float porcentaje = (voltajeLeido - voltMin) * 100.0 / (voltMax - voltMin);

  // 4. Restringir el resultado entre 0 y 100
  if (porcentaje > 100) porcentaje = 100;
  if (porcentaje < 0)   porcentaje = 0;

  return porcentaje;
 
}

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
  pinMode(BATTERY_PIN, INPUT); // Configurar pin de batería
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
    long rssi = WiFi.RSSI(); 
    float voltajeBat = leerVoltajeBateria(); // <-- Leer batería

    if (isnan(humedad) || isnan(temperatura)) {
      Serial.println("Error leyendo DHT22");
      return; 
    }

    StaticJsonDocument<256> doc;
    JsonObject variables = doc.createNestedObject("variables");
    
    variables["temperature"] = temperatura;
    variables["humidity"] = humedad;
    variables["rssi"] = rssi;
    variables["battery"] = voltajeBat; // <-- Se agrega al JSON

    char buffer[256];
    serializeJson(doc, buffer);

    if (client.publish(TOPIC_TEL, buffer)) {
      Serial.print("Publicado con éxito. Batería: ");
      Serial.println(voltajeBat);
    } else {
      Serial.println("Error al publicar mensaje.");
    }
  }
}