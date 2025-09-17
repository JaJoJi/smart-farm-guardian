#include <Wire.h>
#include <SimpleTimer.h>   // version ที่คุณลง
#include "secrets.h"

#define SLAVE_ADDR 0x08
#define TRIG1 18
#define ECHO1 19
#define TRIG2 25
#define ECHO2 26

SimpleTimer timer;   // ตัวจับเวลา

long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30 ms
  if (duration == 0) return -1; // timeout / no echo
  long distance = duration * 0.034 / 2; // cm
  return distance;
}

void sendDataToSlave() {
  long d1 = readUltrasonic(TRIG1, ECHO1);
  long d2 = readUltrasonic(TRIG2, ECHO2);

  if (d1 < 0) d1 = 0;
  if (d2 < 0) d2 = 0;
  if (d1 > 65535) d1 = 65535;
  if (d2 > 65535) d2 = 65535;

  Serial.print("Sending D1="); Serial.print(d1);
  Serial.print(" D2="); Serial.println(d2);

  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(lowByte(d1));
  Wire.write(highByte(d1));
  Wire.write(lowByte(d2));
  Wire.write(highByte(d2));
  uint8_t err = Wire.endTransmission();
  delay(5000);  // ให้ slave มีเวลาตอบสนอง
  if (err != 0) {
    Serial.print("I2C endTransmission error: ");
    Serial.println(err);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  // ตั้ง interval 1000 ms
  timer.setInterval(1000);
}

void loop() {
  if (timer.isReady()) {
    sendDataToSlave();
    timer.reset();   // รีเซ็ตนับใหม่
  }
}
