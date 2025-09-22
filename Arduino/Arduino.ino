#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>  // Watchdog
#include <Servo.h>    // Servo Radar

#define SLAVE_ADDR 0x08
LiquidCrystal_I2C lcd(0x27, 16, 2);

// radar
#define TRIG1 2
#define TRIG2 4
#define TRIG3 6
#define TRIG4 10
#define ECHO1 3
#define ECHO2 5
#define ECHO3 7
#define ECHO4 11
// radar servo
#define ServoDrift 9
Servo RadarServo;

volatile unsigned int d1 = 0;
volatile unsigned int d2 = 0;
volatile bool newData = false;

unsigned long lastUpdate = 0;

// Order from Esp32
void receiveEvent(int howMany) {
  if (howMany >= 4) {
    uint8_t d1L = Wire.read();
    uint8_t d1H = Wire.read();
    uint8_t d2L = Wire.read();
    uint8_t d2H = Wire.read();

    d1 = word(d1H, d1L);
    d2 = word(d2H, d2L);

    newData = true;
    lastUpdate = millis();
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

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UNO I2C Ready");
  delay(500);
  lcd.clear();

  // Setup radar
  pinMode(ECHO1, INPUT);
  pinMode(ECHO2, INPUT);
  pinMode(ECHO3, INPUT);
  pinMode(ECHO4, INPUT);

  pinMode(TRIG1, OUTPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(TRIG3, OUTPUT);
  pinMode(TRIG4, OUTPUT);

  // Radar Servo
  RadarServo.attach(ServoDrift, 500, 2500);

  // เปิด watchdog 8 วินาที
  wdt_enable(WDTO_8S);
}

int angle = 0;    // มุมปัจจุบัน
int stepDir = 1;  // ทิศทางการหมุน (1 = ไปข้างหน้า, -1 = ถอยหลัง)

void loop() {
  // ✅ ขยับ servo ทีละ 1 องศา
  for (int angle = 0; angle <= 180; angle++) {
    // อ่าน ultrasonic ทุก sensor
    int dt1 = radar(TRIG1, ECHO1);
    delay(25);
    int dt2 = radar(TRIG2, ECHO2);
    delay(25);
    int dt3 = radar(TRIG3, ECHO3);
    delay(25);
    int dt4 = radar(TRIG4, ECHO4);
    delay(25);


    Serial.print(angle);
    Serial.print(",");
    Serial.print(dt1);
    Serial.print(",");
    Serial.print(dt2);
    Serial.print(",");
    Serial.print(dt3);
    Serial.print(",");
    Serial.print(dt4);
    Serial.println();


    RadarServo.write(angle);
    delay(10);  // รอให้ servo ขยับเล็กน้อย

    wdt_reset();
  }

  // หมุนกลับ 180 -> 0
  for (int angle = 180; angle >= 0; angle--) {
    int dt1 = radar(TRIG1, ECHO1);
    delay(25);
    int dt2 = radar(TRIG2, ECHO2);
    delay(25);
    int dt3 = radar(TRIG3, ECHO3);
    delay(25);
    int dt4 = radar(TRIG4, ECHO4);
    delay(25);


    Serial.print(angle);
    Serial.print(",");
    Serial.print(dt1);
    Serial.print(",");
    Serial.print(dt2);
    Serial.print(",");
    Serial.print(dt3);
    Serial.print(",");
    Serial.print(dt4);
    Serial.println();


    RadarServo.write(angle);
    delay(10);
    wdt_reset();
  }


  // ✅ reset watchdog เฉพาะเมื่อได้รับข้อมูลใหม่
  if (newData) {
    newData = false;  // เคลียร์ flag

    // อัปเดต LCD
    lcd.setCursor(0, 0);
    lcd.print("D1:");
    lcd.print(d1);
    lcd.print("cm");
    lcd.print("           ");  // ลบเศษตัวเลขเก่า

    lcd.setCursor(0, 1);
    lcd.print("D2:");
    lcd.print(d2);
    lcd.print("cm");
    lcd.print("           ");

    // reset watchdog เมื่อ update สำเร็จ
    wdt_reset();
  }

  // ❌ ถ้า ESP32 timeout ไม่ส่ง → newData ไม่ถูก set → watchdog จะไม่ reset → reset บอร์ดเองภายใน 8s
  delay(200);
}
