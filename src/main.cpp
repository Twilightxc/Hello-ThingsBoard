#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

void publishTelemetry();
void publishAttributes();

// --- Configuration ---
char WIFI_SSID[] = "Wokwi-GUEST";
char WIFI_PASSWORD[] = "";      
const char* token = ""; // <-- Replace with your Access Token
const char* thingsboard_server = "demo.thingsboard.io"; // if DNS fail use 104.196.24.70  | thingsboard.cloud | eu.thingsboard.cloud
const int port = 1883;

const char STUDENT_ID[]      = "";   // <-- Replace with your Student ID
const char FIRMWARE_VERSION[] = "1.0"; 

// --- Hardware Pins ---
#define typeDHT DHT22
#define pinDHT 15
#define pinLED 16 

DHT dht(pinDHT, typeDHT);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastDHTReadTime = 0;
const long DHTReadInterval = 5000; 

// --- Prototypes ---
void onMessage(char* topic, byte* payload, unsigned int length);
void connectToMQTTBroker();

void setup() {
  Serial.begin(9600);
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, HIGH);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ Connected to WiFi");

  client.setServer(thingsboard_server, port);
  // *** set Callback receive from Server ***
  client.setCallback(onMessage);
}

void loop() {
  if (!client.connected()) {
    connectToMQTTBroker();
  }
  client.loop();

  publishTelemetry();
}

// --- RPC Callback Function ---
// this function active when Postman/ThingsBoard send API
void onMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("📩 Message arrived [");
  Serial.print(topic);
  Serial.println("]");

  JsonDocument doc;
  deserializeJson(doc, payload, length);

  String methodName = doc["method"]; // "setLed"
  bool params = doc["params"];       // true or false

  if (methodName == "setLed") {
    digitalWrite(pinLED, params ? LOW : HIGH);
    Serial.print("💡 LED Status: ");
    Serial.println(params ? "ON" : "OFF");
  }

  // send Response back (Optional for Two-way RPC)
  String responseTopic = String(topic);
  responseTopic.replace("request", "response");
  client.publish(responseTopic.c_str(), params ? "{\"success\":true}" : "{\"success\":false}");
}

void connectToMQTTBroker() {
  while (!client.connected()) {
    Serial.println("☁️ Connecting to ThingsBoard...");
    if (client.connect("ESP32_Client", token, NULL)) {
      Serial.println("✅ Connected");
      // *** Subscribe topic receive RPC ***
      client.subscribe("v1/devices/me/rpc/request/+");
      publishAttributes();
    } else {
      delay(5000);
    }
  }
}

void publishTelemetry() {
  if (millis() - lastDHTReadTime < DHTReadInterval) return;
  lastDHTReadTime = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) return;

  JsonDocument doc; 
  doc["temperature"] = t;
  doc["humidity"] = h;

  String payload;
  serializeJson(doc, payload);
  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("📨 Sent: " + payload);
}

void publishAttributes() {
  JsonDocument doc;
  doc["student_id"] = STUDENT_ID;
  doc["firmware_version"] = FIRMWARE_VERSION;

  String payload;
  serializeJson(doc, payload);

  if (client.publish("v1/devices/me/attributes", payload.c_str())) {
    Serial.println("📨 Attributes sent:");
    Serial.println(payload);
  } else {
    Serial.println("❌ Attributes publish failed");
  }
}
