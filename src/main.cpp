// Hello-ThingsBoard
// Send Student ID as Attributes + Random Value as Telemetry

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ================== CONFIG ==================
namespace Config {
  constexpr char WIFI_SSID[]       = "Wokwi-GUEST";
  constexpr char WIFI_PASSWORD[]   = "";

  constexpr char DEVICE_ID[]       = "";
  constexpr char DEVICE_TOKEN[]    = "qRdIKYqwpoZbNhG3lGCl";   // <-- Replace with your Device Token
  constexpr char DEVICE_PASSWORD[] = "";

  constexpr char SERVER[]          = "demo.thingsboard.io";
  constexpr int  PORT              = 1883;

  constexpr char STUDENT_ID[]      = "6750091";   // <-- Replace with your Student ID
  constexpr char FIRMWARE_VERSION[] = "1.0";   // <-- Replace here with your Firmware Version

  inline String STATUS = "";

}

// ================== GLOBALS ==================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ================== DECLARATIONS ==================
void initWiFi();
bool ensureWiFiConnected();
void ensureMQTTConnected();
void publishTelemetry();
void publishAttributes();
void sendData();

// ================== SETUP ==================
void setup() {
  Serial.begin(9600);
  initWiFi();

  mqttClient.setServer(Config::SERVER, Config::PORT);
  randomSeed(analogRead(34));  // seed RNG for telemetry
}

// ================== LOOP ==================
void loop() {
  if (!ensureWiFiConnected()) return;
  ensureMQTTConnected();

  mqttClient.loop();

  sendData();

  delay(1000); // adjust interval to avoid flooding server
}

// ================== IMPLEMENTATION ==================
void initWiFi() {
  Serial.print("🛜 Connecting to WiFi: ");
  Serial.println(Config::WIFI_SSID);

  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 30000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
  } else {
    Serial.println("\n❌ WiFi connection failed. Restarting...");
    ESP.restart();
  }
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.println("⚠️ WiFi lost! Reconnecting...");
  WiFi.reconnect();

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Reconnected");
    return true;
  }

  Serial.println("\n❌ WiFi reconnect failed");
  return false;
}

void ensureMQTTConnected() {
  while (!mqttClient.connected()) {
    Serial.println("☁️ Connecting to ThingsBoard ...");
    if (mqttClient.connect(Config::DEVICE_ID, Config::DEVICE_TOKEN, Config::DEVICE_PASSWORD)) {
      Serial.println("✅ Connected to ThingsBoard");
      Config::STATUS = "ONLINE"; // set status after successful connect
    } else {
      Serial.print("❌ MQTT connect failed, state=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void publishTelemetry() {
  JsonDocument doc;
  doc["random_value"] = random(0, 100);

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("📨 Telemetry sent:");
    Serial.println(payload);
  } else {
    Serial.println("❌ Telemetry publish failed");
  }
}

void publishAttributes() {
  JsonDocument doc;
  doc["student_id"] = Config::STUDENT_ID;
  doc["firmware_version"] = Config::FIRMWARE_VERSION;
  doc["status"] = Config::STATUS;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish("v1/devices/me/attributes", payload.c_str())) {
    Serial.println("📨 Attributes sent:");
    Serial.println(payload);
  } else {
    Serial.println("❌ Attributes publish failed");
  }
}

void sendData() {
  publishTelemetry();
  publishAttributes();
}
