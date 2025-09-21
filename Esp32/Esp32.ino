// ========== Library ==========
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Keypad_I2C.h>
#include <Keypad.h>

// ========== Slave Address ==========
// #define ARDUINO_ADDR 0x08

// ========== Variables (Outside Function) ==========
int LED_Switch = 0;
int GateServoState = 0;
int PumpAuto = 0;
int PumpPush = 0;

// =========== Ultrasonic PIN ===========
#define TRIG1 18
#define ECHO1 19
#define TRIG2 25
#define ECHO2 26


// ========== DHT11 ==========
// #define DHTPIN 4
// #define DHTTYPE DHT11
// DHT dht(DHTPIN, DHTTYPE);

// ========== Soil Moisture ==========
#define SOIL1 34
#define SOIL2 35

// ========== Pump / LED ==========
#define LED_PIN 2
#define PUMP_PIN 32


// ========== Timer ==========
BlynkTimer timer;

// ========== Blynk Virtual Pins ========== (Waiting)
// LED OFF/ON
// BLYNK_WRITE(V0) 
// {
//   LED_Switch = param.asInt();
//   digitalWrite(LED_PIN, LED_Switch);
// }

// // reset/change password
// // BLYNK_WRITE(V3) {}  

// // Gate Servo
// // BLYNK_WRITE(V8) {}

// // PumpAuto Mode
// BLYNK_WRITE(V9) 
// {
//   PumpAuto = param.asInt();
// }

// // Pump Push Switch
// BLYNK_WRITE(V10) {
//   if (!PumpAuto) {  // only allow manual if Auto is off
//     PumpPush = param.asInt();
//     digitalWrite(PUMP_PIN, PumpPush);
//   }
// }

// ========== Ultrasonic Function ==========
long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  if (duration == 0) return -1;                   // timeout / no echo
  long distance = duration * 0.034 / 2;           // cm
  return distance;
}

// ========== Soil Analog to Percent Function ==========
int SoilAnalogToPercent(int raw) {
  int percent = map(raw, 4095, 0, 0, 100);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

// ========== Transmission Function ==========
// void sendDataToSlave() {
//   long d1 = readUltrasonic(TRIG1, ECHO1);
//   long d2 = readUltrasonic(TRIG2, ECHO2);

//   if (d1 < 0) d1 = 0;
//   if (d2 < 0) d2 = 0;
//   if (d1 > 65535) d1 = 65535;
//   if (d2 > 65535) d2 = 65535;

//   Serial.print("Sending D1=");
//   Serial.print(d1);
//   Serial.print(" D2=");
//   Serial.println(d2);

//   Wire.beginTransmission(SLAVE_ADDR);
//   Wire.write(lowByte(d1));
//   Wire.write(highByte(d1));
//   Wire.write(lowByte(d2));
//   Wire.write(highByte(d2));
//   uint8_t err = Wire.endTransmission();
//   delay(5000);  // ให้ slave มีเวลาตอบสนอง
//   if (err != 0) {
//     Serial.print("I2C endTransmission error: ");
//     Serial.println(err);
//   }
// }

// ========== readSensor Function ==========
void readSensor() {
  // DHT11
  // float h = dht.readHumidity();
  // float t = dht.readTemperature();
  // if (!isnan(h) && !isnan(t)) {
  //   // Blynk.virtualWrite(V1, h);
  //   // Blynk.virtualWrite(V2, t);
  // }

  // Soil
  int soilRaw1 = analogRead(SOIL1);
  int soilRaw2 = analogRead(SOIL2);

  int soilPercent1 = SoilAnalogToPercent(soilRaw1);
  int soilPercent2 = SoilAnalogToPercent(soilRaw2);
  Blynk.virtualWrite(V4, soilPercent1);
  Blynk.virtualWrite(V5, soilPercent2);
  // Serial.println(s1);
  // Serial.println(s2);

  // Ultrasonic
  long d1 = readUltrasonic(TRIG1, ECHO1);
  long d2 = readUltrasonic(TRIG2, ECHO2);
  if (d1 < 0) d1 = 0;
  if (d2 < 0) d2 = 0;
  Blynk.virtualWrite(V6, d1);
  Blynk.virtualWrite(V7, d2);
  // Serial.print("D1= ");
  // Serial.print(d1);
  // Serial.print(" D2= ");
  // Serial.println(d2);

  // Pump Auto
  if (PumpAuto) {
    if (soilPercent1 < 50 || soilPercent2 < 50) {  // สมมติ threshold
      digitalWrite(PUMP_PIN, HIGH);
    } else {
      digitalWrite(PUMP_PIN, LOW);
    }
  }
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  // Wire.begin(21, 22);  // SDA, SCL

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  // ตั้ง interval 5000 ms
  timer.setInterval(5000, readSensor);
}

// ========== Loop ==========
void loop() {
  Blynk.run();
  timer.run();
  // if (timer.isReady()) {
  //   // sendDataToSlave();
  //   timer.reset();  // รีเซ็ตนับใหม่
  // }

}
