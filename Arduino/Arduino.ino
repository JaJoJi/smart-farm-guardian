#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>  // Watchdog

#define SLAVE_ADDR 0x08
LiquidCrystal_I2C lcd(0x27, 16, 2);

// radar
#define TRIG1 0
#define TRIG2 2
#define TRIG3 10
#define TRIG4 12
#define ECHO1 1
#define ECHO2 3
#define ECHO3 11
#define ECHO4 13

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
  while (Wire.available()) Wire.read(); // flush
}

// Radar
long radar(int trig,int echo){
  digitalWrite(trig,LOW);
  delay(2);
  digitalWrite(trig,HIGH);
  delay(10);
  digitalWrite(trig,LOW);
  long duration = pulseIn(echo,HIGH);
  int distance = duration*0.034/2; //speed of sound 340m/s or 0.034cm/ms
  return distance;
}

void setup() {
  // Serial Monitor for radar
  Serial.Begin(9600);

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

  // เปิด watchdog 8 วินาที
  wdt_enable(WDTO_8S);
}

void loop() {
  // ✅ reset watchdog เฉพาะเมื่อได้รับข้อมูลใหม่
  int dt1 = radar(TRIG1,ECHO1);
  int dt2 = radar(TRIG2,ECHO2);
  int dt3 = radar(TRIG3,ECHO3);
  int dt4 = radar(TRIG4,ECHO4);

  Serial.println(dt1);
  Serial.println(dt2);
  Serial.println(dt3);
  Serial.println(dt4);

  if (newData) {
    newData = false; // เคลียร์ flag

    // อัปเดต LCD
    lcd.setCursor(0, 0);
    lcd.print("D1:");
    lcd.print(d1);
    lcd.print("cm");
    lcd.print("           "); // ลบเศษตัวเลขเก่า

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
