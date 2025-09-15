#include <Servo.h>

Servo servo1;  // Vertical servo
Servo servo2;  // Horizontal servo

int servoPin1 = 10;  
int servoPin2 = 9;  

int angle1 = 90; // 0-180 range for vertical
int angle2 = 65; // 0-130 range for horizontal

const int angle1Center = 90;  // Center position for vertical
const int angle2Center = 65;  // Center position for horizontal

void setup() {
  servo1.attach(servoPin1);
  servo2.attach(servoPin2);

  // Write initial inverted position
  servo1.write(180 - angle1);  
  servo2.write(130 - angle2);  

  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'U') {          // Up → decrease vertical angle
      angle1 = constrain(angle1 - 2, 0, 180);
    } else if (cmd == 'D') {   // Down → increase vertical angle
      angle1 = constrain(angle1 + 2, 0, 180);
    } else if (cmd == 'L') {   // Left → decrease horizontal angle
      angle2 = constrain(angle2 - 2, 0, 130);
    } else if (cmd == 'R') {   // Right → increase horizontal angle
      angle2 = constrain(angle2 + 2, 0, 130);
    } else if (cmd == 'C') {   // Center → reset both to center
      angle1 = angle1Center;
      angle2 = angle2Center;
    }

    // Write inverted servo positions
    servo1.write(180 - angle1);  
    servo2.write(130 - angle2);

    delay(20); // Prevent serial buffer overflow
  }
}
