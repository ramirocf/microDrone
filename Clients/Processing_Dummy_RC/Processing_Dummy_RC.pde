import processing.net.*;

PFont f;
HScrollbar hsYaw, hsPitch, hsRoll, hsThrottle;
CommunicationsController drone;
void setup() {
  f = createFont("Arial",16,true);
  size(640, 360);
  noStroke();
  hsYaw = new HScrollbar(0, height/2 - 100, width, 16, 16, "yaw");
  hsPitch = new HScrollbar(0, height/2 - 20, width, 16, 16, "pitch");
  hsRoll = new HScrollbar(0, height/2 + 70, width, 16, 16, "roll");
  hsThrottle = new HScrollbar(0, height/2 + 150, width, 16, 16, "throttle");
  drone = new CommunicationsController(this);
}

void draw() {
  background(255);
  textFont(f,16); 

  hsYaw.update();
  hsPitch.update();
  hsRoll.update();
  hsThrottle.update();
  hsYaw.display();
  hsPitch.display();
  hsRoll.display();
  hsThrottle.display();
  drone.sendValues(hsYaw.getPosNormalized(), 
                  hsPitch.getPosNormalized(),
                  hsRoll.getPosNormalized(),
                  hsThrottle.getPosNormalized());
  stroke(0);
}


class HScrollbar {
  int swidth, sheight;    // width and height of bar
  float xpos, ypos;       // x and y position of bar
  float spos, newspos;    // x position of slider
  float sposMin, sposMax; // max and min values of slider
  int loose;              // how loose/heavy
  boolean over;           // is the mouse over the slider?
  boolean locked;
  float ratio;
  String hsName;

  HScrollbar (float xp, float yp, int sw, int sh, int l, String name) {
    swidth = sw;
    sheight = sh;
    int widthtoheight = sw - sh;
    ratio = (float)sw / (float)widthtoheight;
    xpos = xp;
    ypos = yp-sheight/2;
    spos = 0;
    newspos = spos;
    sposMin = xpos;
    sposMax = xpos + swidth - sheight;
    loose = l;
    hsName = name;
  }

  void update() {
    if (overEvent()) {
      over = true;
    } else {
      over = false;
    }
    if (mousePressed && over) {
      locked = true;
    }
    if (!mousePressed) {
      locked = false;
    }
    if (locked) {
      newspos = constrain(mouseX-sheight/2, sposMin, sposMax);
    }
    if (abs(newspos - spos) > 1) {
      spos = spos + (newspos-spos)/loose;
    }
  }

  float constrain(float val, float minv, float maxv) {
    return min(max(val, minv), maxv);
  }

  boolean overEvent() {
    if (mouseX > xpos && mouseX < xpos+swidth &&
       mouseY > ypos && mouseY < ypos+sheight) {
      return true;
    } else {
      return false;
    }
  }

  void display() {
    noStroke();
    fill(204);
    rect(xpos, ypos, swidth, sheight);
    if (over || locked) {
      fill(0, 0, 0);
    } else {
      fill(102, 102, 102);
    }
    rect(spos, ypos, sheight, sheight);
    writeText();
  }

  void writeText() {
    text(hsName, width/2, ypos - 30);
    text("0", 10, ypos - 10);
    text("255", width-30, ypos - 10); 
    text(getPosNormalized(), width/2, ypos - 10);
  }

  float getPos() {
    // Convert spos to be values between
    // 0 and the total width of the scrollbar
    return spos * ratio;
  }
  
  int getPosNormalized() {
      return ((int) (spos / sposMax * 255));
  }
}

void sendDataToMicroDron() {
    
}

class CommunicationsController {
  final static String DRON_HOST = "192.168.4.1";
  final static int DRON_PORT = 23; 
  Client droneClient;
  
  CommunicationsController(PApplet p){
    droneClient = openDroneClient(p, DRON_HOST, DRON_PORT);
  }
  
  Client openDroneClient(PApplet p, String host, int port){
    Client c;
    c = new Client(p, host, port);
    return c;
  }
  
  boolean isReady() {
    return droneClient.active(); 
  }
  
  void sendValues(int yaw, int pitch, int roll, int throttle){
    if (!this.isReady()){
      text("Data cannot be sent: Check connection", width/2 - 60, 20);
      return;
    }
      text("Sending data", width/2 - 30 , 20);
      droneClient.write("\n");
      droneClient.write(yaw);
      droneClient.write(pitch);
      droneClient.write(roll);
      droneClient.write(throttle);
      //println(" " + yaw + " " + pitch + " " + roll + " " + throttle);
      delay(50);
  }
}