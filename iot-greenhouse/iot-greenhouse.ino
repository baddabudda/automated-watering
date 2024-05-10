#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "home.h"
#include "wificonfig.h"

struct WaterStats {
  float level;
  int percent;
};

struct SoilStats {
  int value;
  int percent;
};

/* ===== CONSTANTS =====*/
const int SERIAL_PORT = 115200;
const int RELAY_IN_PIN = 23;

const int ECHO_PIN = 27;
const int TRIG_PIN = 26;

const int SOIL_PIN = 33;
const int ACTIVATOR_PIN = 5;

/* ===== NETWORK =====*/
const char* ssid = "forg";
const char* password =  "amdistrash";

/* ===== INNER PARAMS ===== */
unsigned long duration;
float distance;

/* ===== PREFERENCES ===== */
Preferences prefs;

float waterMax = 22;
int soilMin = 0;
int soilMax = 4095;
int moistThreshold = 2040;
int checkInterval = 10;
int wateringDuration = 3000;

WebServer server(80);
String headerRequest;

int input = 0; // for serial input filter (debug)

/* ===== SYSTEM FUNCTIONS ===== */
WaterStats water_check() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = waterMax - duration * 0.034 / 2;

  int percent = map(distance, 0, waterMax, 0, 100);

  return {distance, percent};
}

SoilStats soil_check() {
  int moistValue;

  digitalWrite(ACTIVATOR_PIN, HIGH);
  delay(1000);
  moistValue = analogRead(SOIL_PIN);
  digitalWrite(ACTIVATOR_PIN, LOW);

  int percent = map(moistValue, soilMin, soilMax, 0, 100);

  return {moistValue, percent};
}

void pump_manager(bool manual=false, int soil=0) {
  if (manual) water_pump();
  if (!manual && soil > moistThreshold) water_pump();
}

void water_pump() {
  digitalWrite(RELAY_IN_PIN, HIGH);
  delay(wateringDuration);
  digitalWrite(RELAY_IN_PIN, LOW);
}

/* ===== REQUEST HANDLERS ===== */

void handle_home() {
  WaterStats waterStats = water_check();
  SoilStats soilStats = soil_check();

  StaticJsonDocument<128> doc;

  JsonObject water = doc.createNestedObject("water");
  water["level"] = waterStats.level;
  water["percent"] = waterStats.percent;

  JsonObject soil = doc.createNestedObject("soil");
  soil["value"] = soilStats.value;
  soil["percent"] = soilStats.percent;
  
  String json;
  serializeJsonPretty(doc, json);
  server.send(200, "application/json", json);
}

void handle_water() {
  pump_manager(true);
  handle_home();
}

void handle_get_config() {
  StaticJsonDocument<192> doc;

  doc["waterMax"] = prefs.getFloat("waterMax", 22);
  doc["soilMin"] = prefs.getInt("soilMin", 0);
  doc["soilMax"] = prefs.getInt("soilMax", 4095);
  doc["moistThreshold"] = prefs.getInt("moistThreshold", 2040);
  doc["checkInterval"] = prefs.getInt("checkInterval", 10);
  doc["wateringDuration"] = prefs.getInt("wateringDuration", 3000) / 1000;

  String json;
  serializeJsonPretty(doc, json);
  server.send(200, "application/json", json);
}

void handle_post_config() {
  String _waterMax = server.arg("waterMax");
  String _soilMin = server.arg("soilMin");
  String _soilMax = server.arg("soilMax");
  String _moistThreshold = server.arg("moistThreshold");
  String _checkInterval = server.arg("checkInterval");
  String _wateringDuration = server.arg("wateringDuration");
  Serial.println(_wateringDuration);
  
  waterMax = _waterMax.toFloat();
  soilMin = _soilMin.toInt();
  soilMax = _soilMax.toInt();
  moistThreshold = _moistThreshold.toInt();
  checkInterval = _checkInterval.toInt();
  wateringDuration = _wateringDuration.toInt();

  prefs.putFloat("waterMax", waterMax);
  prefs.putInt("soilMin", soilMin);
  prefs.putInt("soilMax", soilMax);
  prefs.putInt("moistThreshold", moistThreshold);
  prefs.putInt("checkInterval", checkInterval);
  prefs.putInt("wateringDuration", wateringDuration);

  StaticJsonDocument<48> doc;
  doc["message"] = "success";

  String json;
  serializeJsonPretty(doc, json);
  server.send(200, "application/json", json);
}

void handle_test() {
  StaticJsonDocument<48> doc;
  doc["message"] = "success";

  String json;
  serializeJsonPretty(doc, json);
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(SERIAL_PORT);

  pinMode(RELAY_IN_PIN, OUTPUT);
  digitalWrite(RELAY_IN_PIN, LOW);

  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(SOIL_PIN, INPUT);
  pinMode(ACTIVATOR_PIN, OUTPUT);
  digitalWrite(ACTIVATOR_PIN, LOW);

  prefs.begin("config");
  
  waterMax = prefs.getFloat("waterMax", 22);
  soilMin = prefs.getInt("soilMin", 0);
  soilMax = prefs.getInt("soilMax", 4095);
  moistThreshold = prefs.getInt("moistThreshold", 2040);
  checkInterval = prefs.getInt("checkInterval", 10);
  wateringDuration = prefs.getInt("wateringDuration", 3000);

  Serial.println(waterMax);

  WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS);

  /* if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  } */

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handle_home);
  server.on("/water", HTTP_GET, handle_water);
  server.on("/config", HTTP_GET, handle_get_config);
  server.on("/change", HTTP_GET, handle_post_config);
  server.on("/test", HTTP_GET, handle_test);

  server.begin();
}

unsigned long deltaTime = 0;
unsigned long prevTime = 0;

void loop() {
  server.handleClient();

  deltaTime = millis() - prevTime;

  unsigned long mins = (deltaTime / 60000ul);
  if (mins > checkInterval) {
    SoilStats soil = soil_check();
    pump_manager(soil.value);
    prevTime = millis();
  }

  // int timeMins = (deltaTime / 60ul);
  // if (timeMins > minuteThreshold) {
  //   soil_check();
  //   prevTime = 0;
  // }

  /* if (Serial.available() > 0) {
    input = Serial.read();
    if (input == '1') {
      digitalWrite(ACTIVATOR_PIN, HIGH);
      delay(1000);
      int moistValue = analogRead(SOIL_PIN);
      Serial.println(moistValue);
      digitalWrite(ACTIVATOR_PIN, LOW);
    }
  } */
}
