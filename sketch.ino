#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

// ---------- PIN DEFINITIONS ----------
#define DHTPIN 4
#define DHTTYPE DHT22

#define LDR_PIN 34
#define LED_PIN 5

#define TRIG_PIN 25
#define ECHO_PIN 26

#define RELAY_PIN 18

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ---------- WIFI ----------
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ---------- MQTT ----------
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// ---------- MQTT DATA TOPICS ----------
const char* temperatureTopic = "tanisha/iot/temperature";
const char* humidityTopic    = "tanisha/iot/humidity";
const char* lightTopic       = "tanisha/iot/light";
const char* distanceTopic    = "tanisha/iot/distance";

// ---------- MQTT CONTROL TOPICS ----------
const char* ledTopic   = "tanisha/iot/led";
const char* relayTopic = "tanisha/iot/relay";

// ---------- MQTT STATUS TOPICS ----------
const char* ledStatusTopic   = "tanisha/iot/led/status";
const char* relayStatusTopic = "tanisha/iot/relay/status";

// ---------- OBJECTS ----------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// ---------- WIFI CONNECTION ----------
void connectWiFi() {

  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ---------- MQTT CALLBACK ----------
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.println("MQTT COMMAND RECEIVED");

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  Serial.println(message);

  // ---------- LED CONTROL ----------
  if (String(topic) == ledTopic) {

    if (message == "ON") {

      digitalWrite(LED_PIN, HIGH);

      mqttClient.publish(ledStatusTopic, "ON");

      Serial.println("LED → ON");
      Serial.println("LED Status Published → ON");
    }

    else if (message == "OFF") {

      digitalWrite(LED_PIN, LOW);

      mqttClient.publish(ledStatusTopic, "OFF");

      Serial.println("LED → OFF");
      Serial.println("LED Status Published → OFF");
    }
  }

  // ---------- RELAY CONTROL ----------
  if (String(topic) == relayTopic) {

    if (message == "ON") {

      digitalWrite(RELAY_PIN, HIGH);

      mqttClient.publish(relayStatusTopic, "ON");

      Serial.println("RELAY → ON");
      Serial.println("Relay Status Published → ON");
    }

    else if (message == "OFF") {

      digitalWrite(RELAY_PIN, LOW);

      mqttClient.publish(relayStatusTopic, "OFF");

      Serial.println("RELAY → OFF");
      Serial.println("Relay Status Published → OFF");
    }
  }
}

// ---------- MQTT CONNECTION ----------
void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.println("Connecting to MQTT broker...");

    String clientID = "ESP32-Tanisha-";
    clientID += String(random(0xffff), HEX);

    if (mqttClient.connect(clientID.c_str())) {

      Serial.println("MQTT Connected!");

      mqttClient.subscribe(ledTopic);
      mqttClient.subscribe(relayTopic);

      Serial.println("Subscribed to control topics.");

    }

    else {

      Serial.print("MQTT connection failed, state: ");
      Serial.println(mqttClient.state());

      delay(2000);
    }
  }
}

// ---------- SETUP ----------
void setup() {

  Serial.begin(115200);

  // DHT
  dht.begin();

  // Pin modes
  pinMode(LDR_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);

  // Initial states
  digitalWrite(LED_PIN, LOW);

  digitalWrite(RELAY_PIN, LOW);

  // ---------- OLED ----------
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED initialization failed!");

    while (true);
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("MQTT IoT SYSTEM");

  display.println("Initializing...");

  display.display();

  delay(2000);

  // ---------- WIFI ----------
  connectWiFi();

  // ---------- MQTT ----------
  mqttClient.setServer(mqtt_server, mqtt_port);

  mqttClient.setCallback(mqttCallback);

  Serial.println("--------------------------------");

  Serial.println("ESP32 MQTT IoT SYSTEM");

  Serial.println("--------------------------------");
}

// ---------- MAIN LOOP ----------
void loop() {

  // Make sure MQTT stays connected
  if (!mqttClient.connected()) {

    connectMQTT();
  }

  // Process incoming MQTT messages
  mqttClient.loop();

  // ---------- DHT SENSOR ----------
  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  // ---------- LDR ----------
  int lightLevel = analogRead(LDR_PIN);

  // ---------- ULTRASONIC ----------
  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  float distance = duration * 0.034 / 2;

  // ---------- SERIAL MONITOR ----------
  Serial.println();

  Serial.println("========== MQTT IoT MONITOR ==========");

  Serial.print("Temperature : ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity    : ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Light Level : ");
  Serial.println(lightLevel);

  Serial.print("Distance    : ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("LED         : ");
  Serial.println(
    digitalRead(LED_PIN) ? "ON" : "OFF"
  );

  Serial.print("Relay       : ");
  Serial.println(
    digitalRead(RELAY_PIN) ? "ON" : "OFF"
  );

  Serial.println("======================================");

  // ---------- MQTT SENSOR PUBLISH ----------
  mqttClient.publish(
    temperatureTopic,
    String(temperature).c_str()
  );

  mqttClient.publish(
    humidityTopic,
    String(humidity).c_str()
  );

  mqttClient.publish(
    lightTopic,
    String(lightLevel).c_str()
  );

  mqttClient.publish(
    distanceTopic,
    String(distance).c_str()
  );

  Serial.println("MQTT Data Published");

  // ---------- OLED ----------
  display.clearDisplay();

  display.setCursor(0, 0);

  display.println("MQTT IoT");

  display.print("T: ");
  display.print(temperature, 1);
  display.println(" C");

  display.print("H: ");
  display.print(humidity, 1);
  display.println(" %");

  display.print("L: ");
  display.println(lightLevel);

  display.print("D: ");
  display.print(distance, 1);
  display.println(" cm");

  display.print("R: ");
  display.println(
    digitalRead(RELAY_PIN) ? "ON" : "OFF"
  );

  display.display();

  // Publish sensor data every 5 seconds
  delay(5000);
}
