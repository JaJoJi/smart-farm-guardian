import processing.serial.*;

Serial myPort;
float[] d = new float[4];
float angle = 0;

// Color of sensor
color[] sensorColor = { color(0,255,0), color(0,0,255), color(255,0,0), color(255,255,0) };
// offset sensor (Arduino angle 0-180)
float[] offsetAngle = { 270, 180, 0, 90 }; // d1=up, d2=left, d3=right, d4=down

// distance radar
float maxRange = 300; // Veriance rbital

void setup() {
  size(1280, 860);
  myPort = new Serial(this, "COM17", 9600); // COM17 Recieve serial data from Arduino UNO
  myPort.bufferUntil('\n');
  smooth();
  textAlign(CENTER, CENTER);
  textSize(18); // font size
}

void draw() {
  background(0);

  // Circular display radar 
  translate(width/2, height/2);

  stroke(0, 255, 0, 80); 
  noFill();
  for (int r=50; r<=maxRange; r+=50) {
    ellipse(0, 0, r*2, r*2);
  }

  // (radar sweep) sensor
  for (int i=0; i<4; i++) {
    if (d[i] >= -1) { // รวมค่า -1
      float a = radians(90 - angle + offsetAngle[i]); // Sensor Position 
      
      // ถ้า d[i] == -1 ให้แสดงที่ขอบวงนอกสุด
      float displayR = (d[i] >= 0) ? map(d[i], 0, 200, 0, maxRange) : maxRange;

      // sweep line
      stroke(0, 255, 0); 
      line(0, 0, cos(a)*maxRange, sin(a)*maxRange); 

      // detect point
      noStroke();
      fill(sensorColor[i]);
      ellipse(cos(a)*displayR, sin(a)*displayR, 15, 15); 

      // แสดงค่าระยะ cm หรือ "-"
      fill(255);
      String displayText = (d[i] >= 0) ? int(d[i]) + "cm" : "-";
      text(displayText, cos(a)*displayR + 20, sin(a)*displayR);
    }
  }

  // Display sensor
  textSize(16);
  fill(sensorColor[0]);  text("Sensor D1", -width/2 + 80, -height/2 + 30);
  fill(sensorColor[1]);  text("Sensor D2", -width/2 + 80, -height/2 + 60);
  fill(sensorColor[2]);  text("Sensor D3", -width/2 + 80, -height/2 + 90);
  fill(sensorColor[3]);  text("Sensor D4", -width/2 + 80, -height/2 + 120);
}

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line != null) {
    line = trim(line);
    String[] tokens = split(line, ',');
    if (tokens.length == 5) {
      angle = float(tokens[0]);
      for (int i=0; i<4; i++) {
        d[i] = float(tokens[i+1]);
      }
    }
  }
}
