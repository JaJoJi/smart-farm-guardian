#include <Servo.h>

Servo servo1;  
Servo servo2;  

int servoPin1 = 10;  
int servoPin2 = 9;  

int angle1 = 90;   // vertical (0–180)
int angle2 = 65;   // horizontal (10–120)

const int angle1Center = 90;  
const int angle2Center = 65;

// state ปุ่มกด
bool moveUp = false, moveDown = false, moveLeft = false, moveRight = false;

// เก็บมุมล่าสุดที่ส่งไป servo
int lastAngle1 = -999;
int lastAngle2 = -999;

// deadband กันสั่น
const int DEAD_BAND = 2;

void setup() {
  servo1.attach(servoPin1);
  servo2.attach(servoPin2);

  servo1.write(180 - angle1);  
  servo2.write(130 - angle2);  

  Serial.begin(9600);
}

void loop() {
  // อ่านคำสั่งจาก Serial
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "U_ON") moveUp = true;
    if (cmd == "U_OFF") moveUp = false;

    if (cmd == "D_ON") moveDown = true;
    if (cmd == "D_OFF") moveDown = false;

    if (cmd == "L_ON") moveLeft = true;
    if (cmd == "L_OFF") moveLeft = false;

    if (cmd == "R_ON") moveRight = true;
    if (cmd == "R_OFF") moveRight = false;

    if (cmd == "CENTER") {
      angle1 = angle1Center;
      angle2 = angle2Center;
    }

    if (cmd == "NOD") {
      doNod();
    }

    if (cmd == "SHAKE") {
      doShake();
    }
  }

  // อัปเดตมุมตามปุ่มกด
  if (moveUp)    angle1 = constrain(angle1 - 1, 0, 180);
  if (moveDown)  angle1 = constrain(angle1 + 1, 0, 180);
  if (moveLeft)  angle2 = constrain(angle2 - 1, 10, 120);
  if (moveRight) angle2 = constrain(angle2 + 1, 10, 120);

  // ส่งค่าไป Servo เฉพาะเมื่อเปลี่ยนเกิน deadband
  if (abs(angle1 - lastAngle1) > DEAD_BAND) {
    servo1.write(180 - angle1);
    lastAngle1 = angle1;
  }

  if (abs(angle2 - lastAngle2) > DEAD_BAND) {
    servo2.write(130 - angle2);
    lastAngle2 = angle2;
  }

  delay(20); // ควบคุมความเร็ว
}

// -----------------------
// ฟังก์ชันพิเศษ
// -----------------------
void doNod() {
  for (int i = 0; i < 2; i++) {
    servo1.write(180 - (angle1Center + 20));  
    delay(300);
    servo1.write(180 - (angle1Center - 20));  
    delay(300);
  }
  servo1.write(180 - angle1Center); // กลับมาที่กลาง
  angle1 = angle1Center;
}

void doShake() {
  for (int i = 0; i < 2; i++) {
    servo2.write(130 - (angle2Center - 20));  
    delay(300);
    servo2.write(130 - (angle2Center + 20));  
    delay(300);
  }
  servo2.write(130 - angle2Center); // กลับมาที่กลาง
  angle2 = angle2Center;
}
