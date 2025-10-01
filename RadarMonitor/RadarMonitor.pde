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

  translate(width/2, height/2);

  // Radar circles
  stroke(0, 255, 0, 80); 
  noFill();
  for (int r=50; r<=maxRange; r+=50) {
    ellipse(0, 0, r*2, r*2);
  }

  // Cross lines
  stroke(0, 255, 0, 80);
  strokeWeight(1);
  line(-maxRange, 0, maxRange, 0); // (E-W)
  line(0, -maxRange, 0, maxRange); // (N-S)

  boolean alert = false; 
  ArrayList<String> alerts = new ArrayList<String>(); // list for sensors detect too close

  // Radar sweep 
  for (int i=0; i<4; i++) {
    if (d[i] >= -1) { 
      float a = radians(90 - angle + offsetAngle[i]); // Sensor Position 

      float displayR;
      String displayText;

      if (d[i] == -1) {
        displayR = maxRange;
        displayText = "-";
      } else if (d[i] > 200) { 
        displayR = maxRange;
        displayText = "- cm";
      } else {
        displayR = map(d[i], 0, 200, 0, maxRange);
        displayText = int(d[i]) + "cm";
      }

      // sweep line with alert check
      if (d[i] != -1 && d[i] <= 10) {
        stroke(255, 0, 0); // red alert line
        alert = true;
        alerts.add("D" + (i+1)); // add which sensor detected
      } else {
        stroke(0, 255, 0); // normal green line
      }
      line(0, 0, cos(a)*maxRange, sin(a)*maxRange); 

      // detect point
      noStroke();
      fill(sensorColor[i]);
      ellipse(cos(a)*displayR, sin(a)*displayR, 15, 15); 

      // distance(cm)
      fill(255);
      textSize(18);
      text(displayText, cos(a)*displayR + 20, sin(a)*displayR);
    }
  }

  // Label Sensor
  textSize(16);
  fill(sensorColor[0]);  text("Sensor D1", -width/2 + 80, -height/2 + 30);
  fill(sensorColor[1]);  text("Sensor D2", -width/2 + 80, -height/2 + 60);
  fill(sensorColor[2]);  text("Sensor D3", -width/2 + 80, -height/2 + 90);
  fill(sensorColor[3]);  text("Sensor D4", -width/2 + 80, -height/2 + 120);

  // N,E,S,W Direction
  textSize(20);
  fill(0, 255, 0);
  text("N", 0, -maxRange - 20);
  text("S", 0,  maxRange + 40);
  text("E",  maxRange + 20, 5);
  text("W", -maxRange - 40, 5);

  // Alert text
  if (alert) {
    textSize(30);
    fill(255, 0, 0);

    // convert ArrayList -> String[]
    String[] alertArr = alerts.toArray(new String[alerts.size()]);
    String alertText = "!!! ALERT: something too close at " + join(alertArr, ", ") + " !!!";

    text(alertText, 0, -height/2 + 50);
  }
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
