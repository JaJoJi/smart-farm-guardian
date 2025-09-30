// ========== Library ==========
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

// ========== Slave Address ==========
#define ARDUINO_ADDR 0x08
#define Keypad_ADDR 0x20
#define EEPROM_ADDR 0x50
#define RTC_ADDR 0x68

LiquidCrystal_I2C lcdKeypad(0x27, 16, 2);
LiquidCrystal_I2C lcdSensor(0x3F, 16, 2);

// ========== Global Variables ==========
float h;
float t;
int soilPercent1;
int soilPercent2;
long Plant1;
long Plant2;

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

bool showDateTime = false;

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

// ========== Timer ==========
BlynkTimer timer;
tmElements_t tm;

// ========== Timer Function ==========
bool getTime(const char *str) {
  int Hour, Min, Sec;
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  static const char *monthName[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  uint8_t monthIndex;
  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) Serial.write('0');
  Serial.print(number);
}

// ========== Interrupt ==========
#define INT_PIN 27
volatile bool dataReady = false;

// ========== Interrupt  Function ==========
void IRAM_ATTR handleInterrupt() {
  dataReady = true;
}

uint16_t lastValidData[5] = { 0, 0, 0, 0, 0 };  // เก็บค่าล่าสุดที่ถูกต้อง

void requestSensorData() {
  uint16_t d[5];

  int bytes = Wire.requestFrom(ARDUINO_ADDR, 10);
  if (bytes != 10) {
    Serial.printf("Expected 10 bytes, got %d\n", bytes);
    return;  // อ่านไม่ครบ ออกเลย
  }

  bool error = false;
  for (int i = 0; i < 5; i++) {
    int low = Wire.read();
    int high = Wire.read();
    if (low == -1 || high == -1) {
      Serial.printf("I2C data error at index %d (-1 received)\n", i);
      error = true;
      break;
    }
    d[i] = ((uint16_t)high << 8) | (uint16_t)low;
  }

  if (error) {
    // ใช้ค่าล่าสุดแทน
    memcpy(d, lastValidData, sizeof(d));
    Serial.println("Using last valid data instead of corrupted");
  } else {
    // เก็บค่าล่าสุดไว้
    memcpy(lastValidData, d, sizeof(d));
  }

  Serial.printf("D1:%u cm, D2:%u cm, D3:%u cm, D4:%u cm, Angle:%u deg\n",
                d[0], d[1], d[2], d[3], d[4]);

  // ===== ตรวจสอบทิศ =====
  String direction = "Unknown";
  if (d[0] <= 10 && d[4] < 45) {
    direction = "North-East";
    Blynk.logEvent("north_radar_detected", "Direction: " + direction);
  } else if (d[0] <= 10 && d[4] < 135) {
    direction = "North";
    Blynk.logEvent("north_radar_detected", "Direction: " + direction);
  } else if (d[0] <= 10 && d[4] <= 180) {
    direction = "North-West";
    Blynk.logEvent("north_radar_detected", "Direction: " + direction);
  } else if (d[1] <= 10 && d[4] < 45) {
    direction = "North-West";
    Blynk.logEvent("west_radar_detected", "Direction: " + direction);
  } else if (d[1] <= 10 && d[4] < 135) {
    direction = "West";
    Blynk.logEvent("west_radar_detected", "Direction: " + direction);
  } else if (d[1] <= 10 && d[4] <= 180) {
    direction = "South-West";
    Blynk.logEvent("west_radar_detected", "Direction: " + direction);
  } else if (d[2] <= 10 && d[4] < 45) {
    direction = "South-East";
    Blynk.logEvent("east_radar_detected", "Direction: " + direction);
  } else if (d[2] <= 10 && d[4] < 135) {
    direction = "East";
    Blynk.logEvent("east_radar_detected", "Direction: " + direction);
  } else if (d[2] <= 10 && d[4] <= 180) {
    direction = "North-East";
    Blynk.logEvent("east_radar_detected", "Direction: " + direction);
  } else if (d[3] <= 10 && d[4] < 45) {
    direction = "South-West";
    Blynk.logEvent("south_radar_detected", "Direction: " + direction);
  } else if (d[3] <= 10 && d[4] < 135) {
    direction = "South";
    Blynk.logEvent("south_radar_detected", "Direction: " + direction);
  } else if (d[3] <= 10 && d[4] <= 180) {
    direction = "South-East";
    Blynk.logEvent("south_radar_detected", "Direction: " + direction);
  } else {
    return;  // ไม่มีวัตถุในระยะ 10 cm

    Serial.println(direction);
  }
}
// ========================= SHOW DATETIME =========================
void updateDateTimeOnLCD() {
  tmElements_t tmLocal;
  if (RTC.read(tmLocal)) {
    char buf1[17], buf2[17];
    int year = tmYearToCalendar(tmLocal.Year) + 543;
    sprintf(buf1, "%02d/%02d/%04d", tmLocal.Day, tmLocal.Month, year);
    sprintf(buf2, "%02d:%02d:%02d", tmLocal.Hour, tmLocal.Minute, tmLocal.Second);
    lcdKeypad.setCursor(0, 0);
    lcdKeypad.print("Date: ");
    lcdKeypad.print(buf1);
    lcdKeypad.setCursor(0, 1);
    lcdKeypad.print("Time: ");
    lcdKeypad.print(buf2);
  } else {
    lcdKeypad.clear();
    lcdKeypad.setCursor(0, 0);
    lcdKeypad.print("RTC Error!");
  }
}

// ========== Blynk Virtual ==========
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
  if (gateStatus == 1) {
    passwordAccepted = true;  // ยืนยันว่าเข้าสู่โหมดใช้งาน
    lcdKeypad.clear();
    lcdKeypad.setCursor(0, 0);
    lcdKeypad.print("Gate Opened");
  } else {
    passwordAccepted = false;  // ออกจากโหมด
    inputPassword = "";
    lcdKeypad.clear();
    lcdKeypad.setCursor(0, 0);
    lcdKeypad.print("Gate Closed");
    delay(1000);
    lcdKeypad.clear();
    lcdKeypad.setCursor(0, 0);
    lcdKeypad.print("Enter Password:");
  }
}

// PumpAuto Mode
BLYNK_WRITE(V9) {  // Pump Auto
  PumpAuto = param.asInt();
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
      passwordAccepted = false;                // reset
    } else if (key == '#') {                   // ยืนยัน
      String savedPass = eepromReadString(0);  // อ่านจาก EEPROM
      if (inputPassword == savedPass) {
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Correct Password");
        gateStatus = 1;
        Blynk.virtualWrite(V8, 1);
        passwordAccepted = true;  // ยืนยันว่าเข้าสู่โหมดใช้งาน
        sendDataToArduino(1, gateStatus);

        if (RTC.read(tm)) {
          int currentHour = tm.Hour;
          if (currentHour >= 18 || currentHour < 5) {
            LEDStatus = 1;
          } else {
            LEDStatus = 0;
          }
          Blynk.virtualWrite(V0, LEDStatus);
          sendDataToArduino(0, LEDStatus);
        }
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
    } else if (key == 'B') {
      if (showDateTime) {
        showDateTime = false;
        lcdKeypad.clear();
        lcdKeypad.setCursor(0, 0);
        lcdKeypad.print("Enter Password:");
      } else {
        showDateTime = true;
        updateDateTimeOnLCD();
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
        sendDataToArduino(1, gateStatus);
        LEDStatus = 0;  // สลับสถานะ
        Blynk.virtualWrite(V0, LEDStatus);
        sendDataToArduino(0, LEDStatus);
      }

      if (key == 'A') {
        LEDStatus = !LEDStatus;  // สลับสถานะ
        Blynk.virtualWrite(V0, LEDStatus);
        sendDataToArduino(0, LEDStatus);
      }
      // else if (key == 'C') {
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
  if (showDateTime) {
    updateDateTimeOnLCD();
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

    if (lcdPage == 0) {  // DHT11
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
    } else if (lcdPage == 1) {  // Soil
      soilPercent1 = SoilAnalogToPercent(analogRead(SOIL1));
      soilPercent2 = SoilAnalogToPercent(analogRead(SOIL2));
      lcdSensor.clear();
      lcdSensor.setCursor(0, 0);
      lcdSensor.print("SoilP1 : ");
      lcdSensor.print(soilPercent1);
      lcdSensor.setCursor(0, 1);
      lcdSensor.print("SoilP2 : ");
      lcdSensor.print(soilPercent2);
    } else if (lcdPage == 2) {
      Plant1 = readUltrasonic(TRIG1, ECHO1);
      Plant2 = readUltrasonic(TRIG2, ECHO2);
      lcdSensor.clear();
      lcdSensor.setCursor(0, 0);
      lcdSensor.print("Plant1 = ");
      lcdSensor.print(15 - Plant1);
      lcdSensor.print(" cm");
      lcdSensor.setCursor(0, 1);
      lcdSensor.print("Plant2 = ");
      lcdSensor.print(15 - Plant2);
      lcdSensor.print(" cm");
      if (Plant1 <= 3) {
        Blynk.logEvent("harvest_time", "Let's Harvest! Plant1 : " + String(15 - Plant1) + " cm");
      }
      if (Plant2 <= 3) {
        Blynk.logEvent("harvest_time", "Let's Harvest! Plant2 : " + String(15 - Plant2) + " cm");
      }
    }
    Blynk.virtualWrite(V1, h);
    Blynk.virtualWrite(V2, t);
    Blynk.virtualWrite(V4, soilPercent1);
    Blynk.virtualWrite(V5, soilPercent2);
    Blynk.virtualWrite(V6, 15 - Plant1);
    Blynk.virtualWrite(V7, 15 - Plant2);
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

  bool parse = false;
  bool config = false;  // ตั้งเวลา DS1307 จากเวลาคอมไพล์
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    if (RTC.write(tm)) { config = true; }
  }

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  // pinMode(LED_PIN, OUTPUT);
  // pinMode(PUMP_PIN, OUTPUT);

  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), handleInterrupt, RISING);

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

  if (dataReady) {
    dataReady = false;
    requestSensorData();
  }

  if (PumpAuto == 1) {
    if ((soilPercent1 + soilPercent2) / 2 < 45) {
      PumpStatus = 1;
    } else {
      PumpStatus = 0;
    }
    sendDataToArduino(2, PumpStatus);
  }
}
