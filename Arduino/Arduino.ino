#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>  // Watchdog
#include <Servo.h>    // Servo Radar

#define SLAVE_ADDR 0x08
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// Radar
#define TRIG1 2
#define TRIG2 4
#define TRIG3 6
#define TRIG4 10
#define ECHO1 3
#define ECHO2 5
#define ECHO3 7
#define ECHO4 11

// Radar Servo
#define ServoDrift 9
Servo RadarServo;
volatile uint8_t dt1 = 0, dt2 = 0, dt3 = 0, dt4 = 0, angle = 0;  // Present Angle;
int stepDir = 1; // Rotate Direction (1 = forward, -1 = reverse)

// Door Lock Servo
#define ServoDrive A2
Servo LockServo;
volatile uint8_t Lock1 = 0;  // global lock1

// Buzzer Alarm
#define BuzzerAlarm 8
// WaterPump
#define WaterPump A0
// LED1
#define LordFai A1
// Send Logic ISR Esp32 from Arduino
#define SendISRtoESP32 A3
unsigned long pulseStart = 0;
bool pulseActive = false;
const unsigned long pulseDuration = 500;  // ms

volatile bool newData = false;
unsigned long lastUpdate = 0;

// Order from Esp32
// get data of value from ESP32
void receiveEvent(int howMany) {
  if (howMany >= 2) {
    byte type = Wire.read();   // Byte1
    byte value = Wire.read();  // Byte2
    newData = true;
    lastUpdate = millis();

    switch (type) {
      case 0:  // LED
        digitalWrite(LordFai, value);
        Serial.println(value);
        break;

      case 1:  // Gate
        Lock1 = value;
        Serial.println(Lock1);
        // Update DoorLock
        if (Lock1 == 1) {
          LockServo.write(70);
          tone(BuzzerAlarm, 1000, 200);  // Beebz 1kHz for 200ms
          wdt_reset();
        } else {
          LockServo.write(0);
          tone(BuzzerAlarm, 500, 1000);  // Beebz 500Hz for 500ms
          wdt_reset();
        }
        break;

      case 2:  // Pump
        digitalWrite(WaterPump, value);
        Serial.println(value);
        break;
    }
  }
  while (Wire.available()) Wire.read();  // flush
}

// get order to send data to ESP32
void GetRequest() {
  Wire.write(lowByte(dt1));
  Wire.write(highByte(dt1));
  Wire.write(lowByte(dt2));
  Wire.write(highByte(dt2));
  Wire.write(lowByte(dt3));
  Wire.write(highByte(dt3));
  Wire.write(lowByte(dt4));
  Wire.write(highByte(dt4));
  Wire.write(lowByte(angle));
  Wire.write(highByte(angle));
}

// Radar
long radar(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(5);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 40000);  // timeout 40ms
  if (duration == 0) return -1;                // no echo
  int distance = duration * 0.034 / 2;
  return distance;
}

void setup() {
  // Serial Monitor for radar
  Serial.begin(9600);

  // I2C Esp32-Arduino
  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(GetRequest);

  // Interrupt to ESP32
  pinMode(SendISRtoESP32, OUTPUT);  // ขา output ไป ESP32

  // Setup radar
  pinMode(ECHO1, INPUT);
  pinMode(ECHO2, INPUT);
  pinMode(ECHO3, INPUT);
  pinMode(ECHO4, INPUT);

  pinMode(TRIG1, OUTPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(TRIG4, OUTPUT);

  //BuzzerAlarm
  pinMode(BuzzerAlarm, OUTPUT);

  //LED
  pinMode(LordFai, OUTPUT);

  // Pump
  pinMode(WaterPump, OUTPUT);

  //Door Lock Servo
  // Initialize Lock1 before attach
  Lock1 = 0;

  // Attach Servo after Lock1 with initialize
  LockServo.attach(ServoDrive);
  delay(50);  // Let's Servo have settle time
  LockServo.write(0);

  // Radar Servo
  RadarServo.attach(ServoDrift, 500, 2500);
  delay(50);
  RadarServo.write(0);
  // เปิด watchdog 8 วินาที
  wdt_enable(WDTO_8S);
}

void loop() {
  // Move servo foreach 1 degree
  // อ่าน Radar ทุก sensor
  dt1 = radar(TRIG1, ECHO1);
  wdt_reset();
  delay(30);
  dt2 = radar(TRIG2, ECHO2);
  wdt_reset();
  delay(30);
  dt3 = radar(TRIG3, ECHO3);
  wdt_reset();
  delay(30);
  dt4 = radar(TRIG4, ECHO4);
  wdt_reset();
  delay(30);

  // ขยับ Servo
  RadarServo.write(angle);
  delay(10);

  // ตรวจจับวัตถุ
  if ((dt1 != -1 && dt1 <= 10) || (dt2 != -1 && dt2 <= 10) || (dt3 != -1 && dt3 <= 10) || (dt4 != -1 && dt4 <= 10)) {
    tone(BuzzerAlarm, 1000, 200);
    if (!pulseActive) {
      digitalWrite(SendISRtoESP32, HIGH);
      pulseStart = millis();
      pulseActive = true;
    }
  }

  if (pulseActive && (millis() - pulseStart >= pulseDuration)) {
    digitalWrite(SendISRtoESP32, LOW);
    pulseActive = false;
  }

  // เปลี่ยนค่า angle
  angle += stepDir;
  if (angle <= 0) {
    angle = 0;
    stepDir = +1;
  }  // ถึง 0° → เปลี่ยนทิศ
  if (angle >= 180) {
    angle = 180;
    stepDir = -1;
  }  // ถึง 180° → เปลี่ยนทิศ

  // 🔹 เช็คคำสั่งจาก ESP32
  if (newData) {
    newData = false;  // Clear flag

    if (Lock1 == 1) {
      LockServo.write(70);
      tone(BuzzerAlarm, 1000, 200);
    } else {
      LockServo.write(0);
      tone(BuzzerAlarm, 500, 1000);
    }
  }

  wdt_reset();  // กัน timeout
}

