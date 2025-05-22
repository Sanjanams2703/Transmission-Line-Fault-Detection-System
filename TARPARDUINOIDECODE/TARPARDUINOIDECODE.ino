#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>


const char* ssid = "Hotspot";
const char* pass = "g7puugen";
unsigned long channelID = 2737264;
const char* writeAPIKey = "0BS3N8MQF1LI9ZUC";

WiFiClient client;
SoftwareSerial gpsSerial(D6, D7); 
TinyGPSPlus gps;

const int currentSensorPin = A0;
const float sensitivity = 185.0;
const int zeroCurrentVoltage = 512;

int flamePin = D1;
int bulbPin = D2;
int trigPin = D3;
int echoPin = D4;
int thresholdDistance = 15;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Serial.println("Initializing GPS...");

  connectToWiFi();
  ThingSpeak.begin(client);

  pinMode(flamePin, INPUT);
  pinMode(bulbPin, OUTPUT);
  digitalWrite(bulbPin, LOW);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void connectToWiFi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  float latitude = 0.0, longitude = 0.0;
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {  
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      Serial.write(c);  
      gps.encode(c);
    }
    if (gps.location.isValid()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      break;  
    }
  }

  if (!gps.location.isValid()) {
    Serial.println("\nNo GPS fix yet...");
  }

  
  int flameDetected = digitalRead(flamePin);
  int sensorValue = analogRead(currentSensorPin);
  float voltage = (sensorValue / 1023.0) * 5000;
  float current = (voltage - zeroCurrentVoltage) / sensitivity;

  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.034) / 2;

  
  Serial.print("\nCurrent Level: ");
  Serial.print(current);
  Serial.println(" Amps");

  Serial.print("Flame Detection: ");
  Serial.println(flameDetected == LOW? "Detected" : "Not Detected");

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("GPS: Lat=");
  Serial.print(latitude, 6);
  Serial.print(" Lon=");
  Serial.println(longitude, 6);

  
  ThingSpeak.setField(1, current);
  ThingSpeak.setField(2, flameDetected);
  ThingSpeak.setField(3, distance);
  ThingSpeak.setField(4, (float)latitude);  
  ThingSpeak.setField(5, (float)longitude); 

  int response = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.print("Error sending data: ");
    Serial.println(response);
  }

  
  if (current > 11.50 || flameDetected == LOW || distance < thresholdDistance) {
    digitalWrite(bulbPin, HIGH);
    Serial.println("Condition met: Bulb turned on.");
  } else {
    digitalWrite(bulbPin, LOW);
    Serial.println("Conditions not met: Bulb turned off.");
  }

  delay(5000);
}
