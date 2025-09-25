// ========== Library ==========
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "DHT.h"
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>

// ========== Slave Address ==========
#define ARDUINO_ADDR 0x08
#define Keypad_ADDR 0x20
#define EEPROM_ADDR 0x50

LiquidCrystal_I2C lcdKeypad(0x3F, 16, 2);
LiquidCrystal_I2C lcdSensor(0x27, 16, 2);

// ========== Variables (Outside Function) ==========
float h; 
float t;
int soilPercent1; 
int soilPercent2;
long d1;
long d2; 

int LEDStatus = 0;
int gateStatus = 0;
int PumpStatus = 0;
int PumpAuto = 0;
int PumpPush = 0;

const int PASS_LENGTH = 4;
bool waitingPassword = false;
bool passwordAccepted = false;
String correctPassword = "1234";
String inputPassword = "";

unsigned long lastUpdate = 0;
const long updateInterval = 5000;
int lcdPage = 0;

// =========== Ultrasonic PIN ===========
#define TRIG1 18
#define ECHO1 19
#define TRIG2 25
#define ECHO2 26


// ========== DHT11 ==========
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

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
BLYNK_WRITE(V0) {
  LEDStatus = param.asInt();
  sendDataToArduino(0, LEDStatus);
}

// reset/change password
BLYNK_WRITE(V3) {
  String newPass = param.asStr();
  if (newPass.length() == PASS_LENGTH) {
    eepromWriteString(0, newPass);
    correctPassword = newPass;
    Serial.print("Password updated via Blynk: ");
    Serial.println(correctPassword);
  } else {
    correctPassword = correctPassword;
  }
}

// Gate Servo
BLYNK_WRITE(V8) {  // Gate Control from Blynk
  gateStatus = param.asInt();
  sendDataToArduino(1, gateStatus);
}

// PumpAuto Mode
BLYNK_WRITE(V9) {   // Pump Auto
  PumpAuto = param.asInt();
  if (PumpAuto == 1) {
    PumpStatus = 1;
  } else {
    PumpStatus = 0;
  }
  sendDataToArduino(2, PumpStatus);
}

// Pump Push Switch
BLYNK_WRITE(V10) {  // Pump Push
  if (PumpAuto == 0) {
    PumpPush = param.asInt();
    if (PumpPush == 1) {
      PumpStatus = 1;
    } else {
      PumpStatus = 0;
    }
    sendDataToArduino(2, PumpStatus);
  } else {
    Blynk.virtualWrite(V10, 0);  // reset ปุ่ม
  }
}

// ========== Keypad Setting ==========
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 7, 6, 5, 4 };
byte colPins[COLS] = { 3, 2, 1, 0 };

Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, Keypad_ADDR);

// ========== Keypad Function ==========
void KeypadGate() {
  char key = keypad.getKey();

  if (key) {
    if (key == '*') {  // ยกเลิก
      inputPassword = "";
      lcdKeypad.clear();
      lcdKeypad.setCursor(0, 0);
      lcdKeypad.print("Cleared Input");
      delay(500);
      lcdKeypad.clear();
      lcdKeypad.setCursor(0, 0);
      lcdKeypad.print("Enter Password:");
      passwordAccepted = false;  // reset

    } else if (key == '#') {                   // ยืนยัน
      String savedPass = eepromReadString(0);  // อ่านจาก EEPROM
      if (inputPassword == savedPass) {
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Correct Password");
        gateStatus = 1;
        Blynk.virtualWrite(V8, 1);
        passwordAccepted = true;  // ยืนยันว่าเข้าสู่โหมดใช้งาน
        sendDataToArduino(1, gateStatus);;
      } else {
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Incorrect");
        lcdKeypad.setCursor(0, 1);
        lcdKeypad.print("Password!");
        inputPassword = "";
        passwordAccepted = false;
        delay(1000);
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Enter Password:");
      }

    } else if (passwordAccepted) {
      // ===== โหมดหลังใส่รหัสถูกต้อง =====
      if (key == 'D') {
        gateStatus = 0;  // ปิดประตู
        Blynk.virtualWrite(V8, 0);
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Gate Closed");
        delay(1000);
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Enter Password:");
        passwordAccepted = false;  // ออกจากโหมด
        inputPassword = "";
        sendDataToArduino(1, gateStatus);;
      }
      // else if (key == 'A' || key == 'B' || key == 'C') {
      //   // ฟังก์ชันอื่น ๆ ของ A, B, C
      //   Serial.print("Key pressed: ");
      //   Serial.println(key);
      // }

    } else {
      // ===== โหมดกำลังใส่รหัส ===== (อนุญาตเฉพาะตัวเลข และจำกัด 4 ตัว)
      if (key >= '0' && key <= '9') {
        if (inputPassword.length() < PASS_LENGTH) {
          inputPassword += key;
          lcdKeypad.setCursor(0, 1);
          lcdKeypad.print(inputPassword);
          Serial.println(inputPassword);
        } else {
          // ถ้าเกิน 4 หลัก ไม่ทำอะไร
          lcdKeypad.setCursor(0, 1);
          lcdKeypad.print(inputPassword);  // แสดงเท่าเดิม
          Serial.println(inputPassword);
        }
      }
    }
  }
}

// ========== EEPROM Function ==========
void eepromWriteString(uint16_t addr, const String &data) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);  // high byte
  Wire.write(addr & 0xFF);         // low byte
  for (size_t i = 0; i < data.length(); i++) {
    Wire.write(data[i]);
  }
  Wire.write('\0');  // null terminator
  Wire.endTransmission();
  delay(10);  // EEPROM write cycle time
}

String eepromReadString(uint16_t addr) {
  String result = "";
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((addr >> 8) & 0xFF);
  Wire.write(addr & 0xFF);
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_ADDR, 32);  // อ่านทีละ 32 byte
  while (Wire.available()) {
    char c = Wire.read();
    if (c == '\0') break;
    result += c;
  }
  return result;
}

void Wait5Sec() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
  }
}

// ========== Ultrasonic Function ==========
long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  if (duration == 0) return -1;          // timeout / no echo
  long distance = duration * 0.034 / 2;  // cm
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
void sendDataToArduino(byte type, byte value) {
  Wire.beginTransmission(ARDUINO_ADDR);
  Wire.write(type);   // Byte1: ประเภท
  Wire.write(value);  // Byte2: ค่า
  Wire.endTransmission();
}

// ========== readSensor Function ==========

void readSensor() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;
    
    if (lcdPage == 0) { // DHT11
      h = dht.readHumidity();
      t = dht.readTemperature();
      if (!isnan(h) && !isnan(t)) {
        lcdSensor.clear();
        lcdSensor.setCursor(0, 0);
        lcdSensor.print("Humid : ");
        lcdSensor.print(h);
        lcdSensor.setCursor(0, 1);
        lcdSensor.print("Temp : ");
        lcdSensor.print(t);
        lcdSensor.print(" C");
      }
    } 
    else if (lcdPage == 1) { // Soil
      soilPercent1 = SoilAnalogToPercent(analogRead(SOIL1));
      soilPercent2 = SoilAnalogToPercent(analogRead(SOIL2));
      lcdSensor.clear();
      lcdSensor.setCursor(0, 0);
      lcdSensor.print("P1 : ");
      lcdSensor.print(soilPercent1);
      lcdSensor.setCursor(0, 1);
      lcdSensor.print("P2 : ");
      lcdSensor.print(soilPercent2);
    } 
    else if (lcdPage == 2) {
      d1 = readUltrasonic(TRIG1, ECHO1);
      d2 = readUltrasonic(TRIG2, ECHO2);
      lcdSensor.clear();
      lcdSensor.setCursor(0, 0);
      lcdSensor.print("D1= ");
      lcdSensor.print(d1);
      lcdSensor.setCursor(0, 1);
      lcdSensor.print("D2= ");
      lcdSensor.print(d2);
    }
    Blynk.virtualWrite(V1, h);
    Blynk.virtualWrite(V2, t);
    Blynk.virtualWrite(V4, soilPercent1);
    Blynk.virtualWrite(V5, soilPercent2);
    Blynk.virtualWrite(V6, d1);
    Blynk.virtualWrite(V7, d2);
    // เปลี่ยนหน้า LCD
    lcdPage++;
    if (lcdPage > 2) lcdPage = 0;
  }
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  dht.begin();
  keypad.begin(makeKeymap(keys));
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  Wire.begin(21, 22);  // SDA, SCL

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  lcdKeypad.begin();
  lcdKeypad.backlight();
  lcdKeypad.clear();
  lcdKeypad.setCursor(0, 0);
  lcdKeypad.print("Enter Password:");

  lcdSensor.begin();
  lcdSensor.backlight();
  lcdSensor.clear();
  // ตั้ง interval 5000 ms
  timer.setInterval(5000, readSensor);
}

// ========== Loop ==========
void loop() {
  Blynk.run();
  timer.run();
  KeypadGate();
}
