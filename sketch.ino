#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "time.h"

// WiFi Credentials
#define WIFI_SSID "Wokwi-GUEST"
#define PASSWORD ""

#define DATABASE_URL "https://upcpre202402si572ws71-default-rtdb.firebaseio.com/.json"

HTTPClient client;

// Distance Sensor setup
#define TRIGGER_PIN 5
#define ECHO_PIN 18

// LED setup
#define GREEN_LED 4
#define RED_LED 2

// LCD setup
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

// Variables
int previous = 0;
String tempText = "";
String payload = "";
String sensorID = "HC001";
char timeStringBuffer[20];
String greenLedOn;
String redLedOn;
String isSafeDistance;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  // LCD Startup
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");

  // WiFi Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.println("Online");
  delay(500);

  // Firebase connection setup
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.println("Connecting to");
  LCD.setCursor(0, 1);
  LCD.println("Firebase...");
  Serial.println("connecting...");
  client.begin(DATABASE_URL);
  int httpResponseCode = client.GET();

  if (httpResponseCode > 0) {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.println("Connected");
    Serial.println("connected, Firebase payload: ");
    payload = client.getString();
    Serial.println(payload);
    Serial.println();
  }

  // LED setup
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Distance Sensor setup
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Print Color Legend
  Serial.println("Green: Safe");
  Serial.println("Red: Unsafe");

  // LED Initialization Routine
  for(int i = 0; i < 5; i++) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }

  // NTP Setup
  configTime(-9000, -9000, "1.south-america.pool.ntp.org");
}

void loop() {
  // Distance Sensor Data Collect
  long distancePulse, distanceMetric;
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, LOW);
  distancePulse = pulseIn(ECHO_PIN, HIGH);
  distanceMetric = (distancePulse/2)/29.1;

  if (previous != distanceMetric) {
    if(distanceMetric > 200 || distanceMetric < 0) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      tempText = "Safe distance: ";
      greenLedOn = "true";
      redLedOn = "false";
      isSafeDistance = "true";
    } else {
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      tempText = "Unsafe distance: ";
      greenLedOn = "false";
      redLedOn = "true";
      isSafeDistance = "false";
    }

    struct tm timeInfo;
    if(!getLocalTime(&timeInfo)) {
      Serial.println("Time Error");
    }
    strftime(timeStringBuffer, sizeof(timeStringBuffer), "%d/%m/%Y %H:%M", &timeInfo);
    Serial.println(String(timeStringBuffer));

    // Status Update
    client.PATCH("{\"Status/Sensors/time\":\"" + String(timeStringBuffer) + "\"}");
    client.PATCH("{\"Status/Led/greenLed\":" + greenLedOn + "}");
    client.PATCH("{\"Status/Led/redLed\":" + redLedOn + "}");
    client.PATCH("{\"Status/Sensors/Distance\":" + String(distanceMetric) + "}");
    client.PATCH("{\"Status/Sensors/safeDistance\":" + isSafeDistance + "}");
    client.PATCH("{\"Status/Sensors/id\":\"" + sensorID + "\"}");

    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(tempText);
    LCD.setCursor(0, 1);
    LCD.print(distanceMetric);
    LCD.println(" cm");

  }

  previous = distanceMetric;

}
