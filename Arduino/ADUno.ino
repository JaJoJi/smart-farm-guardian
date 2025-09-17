#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>  // Watchdog

#define SLAVE_ADDR 0x08
LiquidCrystal_I2C lcd(0x27, 16, 2);

volatile unsigned int d1 = 0;
volatile unsigned int d2 = 0;
volatile bool newData = false;

unsigned long lastUpdate = 0;

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

void setup() {
  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(receiveEvent);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UNO I2C Ready");
  delay(500);
  lcd.clear();

  // เปิด watchdog 8 วินาที
  wdt_enable(WDTO_8S);
}

void loop() {
  // ✅ reset watchdog เฉพาะเมื่อได้รับข้อมูลใหม่
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
