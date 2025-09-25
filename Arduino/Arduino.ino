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
// Door Lock Servo
#define ServoDrive A2
Servo LockServo;
// Buzzer Alarm
#define BuzzerAlarm 8
// WaterPump
#define WaterPump A0
// LED1
#define LordFai A1

volatile bool newData = false;
unsigned long lastUpdate = 0;
volatile uint8_t Lock1 = 0;          // ประกาศเป็น global ตรงนี้
volatile uint8_t PumpActivate1 = 0;  // ประกาศเป็น global ตรงนี้
volatile uint8_t LED1 = 0;           // ประกาศเป็น global ตรงนี้

// Order from Esp32
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
        // อัปเดต DoorLock
        if (Lock1 == 1) {
          LockServo.write(70);
          tone(BuzzerAlarm, 1000, 200);  // บี๊บที่ 1kHz นาน 200ms
          wdt_reset();
        } else {
          LockServo.write(0);
          tone(BuzzerAlarm, 500, 1000);  // บี๊บที่ 500Hz นาน 500ms
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

  // Radar Servo
  RadarServo.attach(ServoDrift, 500, 2500);

  //Door Lock Servo
  LockServo.attach(ServoDrive);

  // เปิด watchdog 8 วินาที
  wdt_enable(WDTO_8S);
  
  LockServo.write(0);
}

int angle = 0;    // มุมปัจจุบัน
int stepDir = 1;  // ทิศทางการหมุน (1 = ไปข้างหน้า, -1 = ถอยหลัง)

void loop() {
  // ✅ ขยับ servo ทีละ 1 องศา
  for (int angle = 0; angle <= 180; angle++) {
    // อ่าน ultrasonic ทุก sensor
    int dt1 = radar(TRIG1, ECHO1);
    delay(60);
    int dt2 = radar(TRIG2, ECHO2);
    delay(60);
    int dt3 = radar(TRIG3, ECHO3);
    delay(60);
    int dt4 = radar(TRIG4, ECHO4);
    delay(60);

    // Serial.print(angle);
    // Serial.print(",");
    // Serial.print(dt1);
    // Serial.print(",");
    // Serial.print(dt2);
    // Serial.print(",");
    // Serial.print(dt3);
    // Serial.print(",");
    // Serial.print(dt4);
    // Serial.println();

    RadarServo.write(angle);
    delay(10);  // รอให้ servo ขยับเล็กน้อย

    if ((dt1 != -1 && dt1 <= 20) || (dt2 != -1 && dt2 <= 20) || (dt3 != -1 && dt3 <= 20) || (dt4 != -1 && dt4 <= 20)) {
      tone(BuzzerAlarm, 1000, 200);  // บี๊บ 1kHz 200ms
    }

    wdt_reset();
  }

  // หมุนกลับ 180 -> 0
  for (int angle = 180; angle >= 0; angle--) {
    int dt1 = radar(TRIG1, ECHO1);
    delay(60);
    int dt2 = radar(TRIG2, ECHO2);
    delay(60);
    int dt3 = radar(TRIG3, ECHO3);
    delay(60);
    int dt4 = radar(TRIG4, ECHO4);
    delay(60);

    // Serial.print(angle);
    // Serial.print(",");
    // Serial.print(dt1);
    // Serial.print(",");
    // Serial.print(dt2);
    // Serial.print(",");
    // Serial.print(dt3);
    // Serial.print(",");
    // Serial.print(dt4);
    // Serial.println();


    RadarServo.write(angle);
    delay(10);

    if ((dt1 != -1 && dt1 <= 20) || (dt2 != -1 && dt2 <= 20) || (dt3 != -1 && dt3 <= 20) || (dt4 != -1 && dt4 <= 20)) {
      tone(BuzzerAlarm, 1000, 200);  // บี๊บ 1kHz 200ms
    }

    wdt_reset();
  }

  // reset watchdog เฉพาะเมื่อได้รับข้อมูลใหม่
  if (newData) {
    newData = false;  // เคลียร์ flag

    // อัปเดต DoorLock
    if (Lock1 == 1) {
      LockServo.write(70);
      tone(BuzzerAlarm, 1000, 200);  // บี๊บที่ 1kHz นาน 200ms
      wdt_reset();
    } else {
      LockServo.write(0);
      tone(BuzzerAlarm, 500, 1000);  // บี๊บที่ 500Hz นาน 500ms
      wdt_reset();
    }
  }

  // ถ้า ESP32 timeout ไม่ส่ง → newData ไม่ถูก set → watchdog จะไม่ reset → reset บอร์ดเองภายใน 8s
  delay(200);
}
