
#include "Credentials.h"
#include <Time.h>
#include <DS1307RTC.h>
#include <Wire.h> 

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "Adafruit_MCP23017.h"

#include <Streaming.h>
//#include <SoftwareSerial.h>
//#include <WiFlySerial.h>
#include <WiFlyHQ.h>
#include <avr/wdt.h> 

#include <ByteBuffer.h>
#include <ooPinChangeInt.h> // necessary otherwise we get undefined reference errors.
#define DEBUG
#ifdef DEBUG
ByteBuffer printBuffer(200);
#endif
#include <AdaEncoder.h>

#define ENCA_a 14
#define ENCA_b 15

AdaEncoder encoderA = AdaEncoder('a', ENCA_a, ENCA_b);
String buffer = String();

int lastClick0 = 0;
int lastClick1 = 0;
int lastClick2 = 0;
int lastClick3 = 0;
int lastClick4 = 0;
int lastClick5 = 0;

//Settings
int minPos = 0;
int maxPos = 1500;

int crackPos = 80;
int crackPosWide = 400;

int overshootUp = 50;
int overshootDown = 30;


#define DEFAULT_OPEN_HR 8
#define DEFAULT_OPEN_MIN 0
#define DEFAULT_CLOSE_HR 18
#define DEFAULT_CLOSE_MIN 0


byte openHr = DEFAULT_OPEN_HR;
byte openMin = DEFAULT_OPEN_MIN;
byte closeHr = DEFAULT_CLOSE_HR;
byte closeMin = DEFAULT_CLOSE_MIN;

#define VAL_BUF_SIZE 32
char valBuffer[VAL_BUF_SIZE];

int pos = 0;
int lastDir = 0;

#define DOOR_CONTROL 1

#define DEST_NONE 0
#define DEST_CLOSE 1
#define DEST_OPEN 2
#define DEST_CRACK 3
#define DEST_CRACK_WIDE 4

int doorDestination = DEST_NONE;

#define LOC_UNK 0
#define LOC_CLOSED 1
#define LOC_OPEN 2
#define LOC_CRACKED 3
#define LOC_CRACKED_WIDE 4
#define LOC_MOVING 5

int doorLocation = LOC_UNK;
//int doorLocation = LOC_CLOSED;


#define DIR_UNK 0
#define DIR_UP 1
#define DIR_DOWN 2

int doorDirection = DIR_UNK;


Adafruit_MCP23017 mcp;


//WiFly
WiFly wifly;



void setup()
{
  delay(1000);
  Serial.begin(9600);
  wifly.begin(&Serial, NULL);
  if (!wifly.isAssociated()) {
	// Setup the WiFly to connect to a wifi network 
	//Serial.println("Joining network");
	wifly.setSSID(WIFI_SSID);
	wifly.setPassphrase(WIFI_PASSPHRASE);
	wifly.enableDHCP();

	if (wifly.join()) {
	    //Serial.println("Joined wifi network");
	} else {
	    //Serial.println("Failed to join wifi network");
	    //terminal();
	}
    } else {
        //Serial.println("Already joined network");
    }
    if (wifly.isConnected()) {
        //Serial.println("Old connection active. Closing");
	wifly.close();
    }
  
  //Serial.begin(115200);
  //Serial.println("Garage Mk2 V2-----------------------------");
//AdaEncoder encoderA = AdaEncoder('a', ENCA_a, ENCA_b);
  //Time 
  setSyncInterval(120);
  setSyncProvider(RTC.get);
  
  //Serial.println(hour());

  //Expander Board
  mcp.begin(0);
  
  mcp.pinMode(DOOR_CONTROL, OUTPUT);
  mcp.digitalWrite(DOOR_CONTROL, LOW);
  //Serial.print(hour());
  //Serial.print(':');
  //Serial.println(minute());
  //Serial.println("Update");
  
  
  forceCloseDoor();
  
  sendUpdate();
}

void loop() 
{
  /*while (Serial.available()) {
    char c = Serial.read();
    
    switch (c) {
      case 'o':
        openDoor();
        break;
      case 'c':
        closeDoor();
        break;
      case 'k':
        crackDoor();
        break;
      case 'f':
        Serial.println("Force Cracked");
        doorLocation = LOC_CRACKED;
        break;
    }
  }*/
  if ((second() == 15) && ((minute() % 56) == 0)) {
    sendUpdate(); 
  }
  
  // Just do this 'occasionally'
  //if ((millis() % 50) == 0) {
  updateEncoder(); //Has 50ms delay in it
  
  if (second() == 0) {
    if (hour() == openHr && minute() == openMin) {
      crackDoor();
      delay(1000);
    } else if (hour() == closeHr && minute() == closeMin) {
      closeDoor();
      delay(1000);
    }
  }
  
  //If we are moving, someone else moved it.
  if (doorDirection != DIR_UNK) {
    //Serial.println("Door Moved");
    //doorLocation = LOC_UNK;
  }
  //}
  
  if ((millis() % 1000) == 0) {
    //updateEncoder();
    //Serial.println(pos);
    
    //doorLocation = LOC_UNK;

  }
}

void openDoor() {
  if (doorLocation == LOC_OPEN) {
    return;
  }
  //Serial.println("Opening Door");
  doorDestination = DEST_OPEN;
  moveDoor();
}

void forceCloseDoor() {
  if (doorLocation == LOC_CLOSED) {
    return;
  }
  //Serial.println("Closing Door");
  doorDestination = DEST_CLOSE;
  moveDoor();
}

void closeDoor() {
  if (doorLocation == LOC_CLOSED) {
    return;
  }
  //Serial.println("Closing Door");
  doorDestination = DEST_CRACK_WIDE;
  moveDoor();
  doorDestination = DEST_CLOSE;
  moveDoor();
}

void crackDoor() {
  if (doorLocation == LOC_CRACKED) {
    return;
  }
  closeDoor();
  //Serial.println("Cracking Door");
  doorDestination = DEST_CRACK_WIDE;
  moveDoor();
  doorDestination = DEST_CRACK;
  moveDoor();
}

void moveDoor() {
  updateEncoder();
  //Serial.println(doorDirection);
  if (doorDirection == DIR_UNK) {
    //Not moving. Start moving.
    Serial.println("Starting");
    pressDoorButton();
    delay(100);
  }
  
  while (1) {
    updateEncoder();
    
    switch (doorDestination) {
      case DEST_CLOSE:
        if (doorDirection == DIR_UP) {
          //Switch Dir to down
          //Serial.println("Revsering Door from up to down");
          delay(1000);
          pressDoorButton();
          delay(3000);
          updateEncoder();
          lastClick5 = 0; lastClick4 = 0; lastClick3 = 0; lastClick2 = 0; lastClick1 = 0; lastClick0 = 0;
          updateEncoder();
          pressDoorButton();
          delay(500);
          updateEncoder();
          lastClick5 = 0; lastClick4 = 0; lastClick3 = 0; lastClick2 = 0; lastClick1 = 0; lastClick0 = 0;
          delay(500);
        } else if (doorDirection == DIR_DOWN) {
          //Still moving down. Do nothing.
          //Serial.println("Still moving");
          delay(500);
        } else if (doorDirection == DIR_UNK) {
          //Stopped
          //pos = 0;
          
          //We are probablly closed? Need to check for bound.
          delay(1000);
          updateEncoder();
          if (doorDirection == DIR_UNK) {
            //Yup, we seem to have stopped.
            //Serial.println("Door is closed");
            doorLocation = LOC_CLOSED;
            pos = 0; //Recalibrate
            return;
          }
        }
        break;
      case DEST_OPEN:
        if (doorDirection == DIR_UP) {
          //Still moving down. Do nothing.
        } else if (doorDirection == DIR_DOWN) {
          //Switch Dir to down
          //Serial.println("Reversing Door");
          pressDoorButton();
          delay(500);
          updateEncoder();
          lastClick5, lastClick4, lastClick3, lastClick2, lastClick1, lastClick0 = 0;
          pressDoorButton();
        } else if (doorDirection == DIR_UNK) {
          //Stopped
          //pos = 0;
          
          //We are probablly closed? Need to check for bound.
          delay(1000);
          updateEncoder();
          if (doorDirection == DIR_UNK) {
            //Yup, we seem to have stopped.
            //Serial.println("Door is open");
            doorLocation = LOC_OPEN;
            return;
          }
        }
        break;
      case DEST_CRACK:
        //if (doorDirection == DIR_UP) {
        if (pos <= (crackPos + overshootDown)) {
          pressDoorButton();
          delay(2000);
          //Serial.println("Door is cracked headed up.");
          updateEncoder();
          lastClick5 = 0; lastClick4 = 0; lastClick3 = 0; lastClick2 = 0; lastClick1 = 0; lastClick0 = 0;
          updateEncoder();
          doorLocation = LOC_CRACKED;
          return;
        }
        break;
      case DEST_CRACK_WIDE:
        if (pos >= (crackPosWide - overshootUp)) {
          pressDoorButton();
          delay(2000);
          //Serial.println("Door is cracked headed up.");
          updateEncoder();
          lastClick5 = 0; lastClick4 = 0; lastClick3 = 0; lastClick2 = 0; lastClick1 = 0; lastClick0 = 0;
          updateEncoder();
          doorLocation = LOC_CRACKED_WIDE;
          return;
        }
        break;
      
          
          //Check Crack
          
        //} else if (doorDirection == DIR_DOWN) {
        /*  if (pos <= (crackPos + overshootDown)) {
            pressDoorButton();
            Serial.println("Door is cracked headed down.");
            doorLocation = LOC_CRACKED;
            return;
          }
        }*/
        
        break;
    }
  }
  
}

void pressDoorButton() {
  //Serial.println("Button Press");
  doorLocation = LOC_MOVING;
  mcp.digitalWrite(DOOR_CONTROL, HIGH);
  delay(250);
  mcp.digitalWrite(DOOR_CONTROL, LOW);
  
}

void updateEncoder() {
  delay(50);
  int clicks = 0;
  AdaEncoder *thisEncoder;
  thisEncoder=AdaEncoder::genie();
  if (thisEncoder != NULL) {
    clicks = thisEncoder->clicks;
    thisEncoder->clicks = 0;
    pos += clicks;
    if (pos < 0) {
      pos = 0; //0 is min, door closed
    }
    if (pos > maxPos) {
      maxPos = pos;
    }
  }
  
  lastClick5 = lastClick4;
  lastClick4 = lastClick3;
  lastClick3 = lastClick2;
  lastClick2 = lastClick1;
  lastClick1 = lastClick0;
  lastClick0 = clicks;
  
  int sumClicks = lastClick0 + lastClick1 + lastClick2 + lastClick3 + lastClick4 + lastClick5;
  //Serial.print(sumClicks);Serial.print(":");Serial.println(clicks);
  if (sumClicks > 4) {
    doorDirection = DIR_UP;
  } else if (sumClicks < -4) {
    doorDirection = DIR_DOWN;
  } else {
    doorDirection = DIR_UNK;
  }
}

void sendUpdate() {  
  makeConnection();
}

void makeConnection() {
  if (wifly.open(REMOTE_SERVER, REMOTE_PORT)) {
    //Serial.print("Connected to ");
    //Serial.println(site);

    // Send the request
    wifly << F("GET ") << REMOTE_URI << F("?key=") << REMOTE_PASSWORD;
    wifly << F("&hour=") << hour() << F("&min=") << minute() << F("&sec=") << second() << F("&pos=") << pos;
    wifly << F("&ch=") << closeHr << F("&cm=") << closeMin << F("&oh=") << openHr << F("&om=") << openMin; 
    
    
    wifly << F(" HTTP/1.1\r\n") << F("HOST: ") << REMOTE_SERVER << endl << endl;
  } else {
    //Serial.println("Failed to connect");
  }
  
  loadResponse();
}

void loadResponse() {
  unsigned long TimeOut = millis() + 40000;
  char c1 = 0;
  char c2 = 0;
  char c3 = 0;
  char c4 = 0;
  char c5 = 0;
  char c6 = 0;
  char c7 = 0;
  char testc2 = 0;
  char testc3 = 0;
  boolean load = false;
  
  tmElements_t tm;
  tm.Year = 0;
  
  if (!wifly.isConnected()) {
    wifly.close();
    //sendStatus("Not connected");
    return; 
  }
  
  int val = 0;
  
  while (  TimeOut > millis() && wifly.isConnected() ) {
    if (  wifly.available() > 0 ) {
      c7 = c6;
      c6 = c5;
      c5 = c4;
      c4 = c3;
      c3 = c2;
      c2 = c1;
      c1 = (char)wifly.read();
      
      
      
      if (c1 == '?' && c2 == '^') {
        load = true;
      }
      
      if (load && c1 == '&') {
        val = ((c3 - '0') * 10) + (c2 - '0');
        testc2 = c2; testc3 = c3;
        
        switch (c5) {
          case 'y':
            tm.Year = CalendarYrToTm(val + 2000);
            break;
          case 'M':
            tm.Month = val;
            break;
          case 'd':
            tm.Day = val;
            break;
          case 'h':
            tm.Hour = val;
            break;
          case 'm':
            tm.Minute = val;
            break;
          case 's':
            tm.Second = val;
            break;
          case 'o':
            openMin = val;
            break;
          case 'O':
            openHr = val;
            break;
          case 'c':
            closeMin = val;
            break;
          case 'C':
            closeHr = val;
            break;
          case 'k':
            crackPos = val;
            break;
        }
      }
      
      //buffer = buffer + (char)wifly.read();
      /*if (load) {
        buffer += (char)wifly.read();
      } else {
        //c = b;
        b = a;
        a = (char)wifly.read();
        if (a == '?' && b == '^') {
          load = true;
          buffer += "^?";
        }
      }*/
    }
  }
  
  //Serial.println(buffer);
  wifly.close();
  //sendStatus(tm.Year);
  if (tm.Year > 0) {
    //sendStatus(tm.Hour);
    RTC.write(tm);
  }
  
  
  //processResponse();
}

/*void processResponse() {
  //String block = buffer.substring(buffer.indexOf("^"), buffer.lastIndexOf("$"));
  
  
  String var;
  String val;
  String tmp;
  byte importSlots = 0;
  
  int index1 = 0;
  int index2 = 0;
  int i = 0;
  int intVal = 0;
  int endOfLine = 0;
  
  index1 = buffer.indexOf('^');
  //sendStatus(String(index1));
  while (!endOfLine && (index1 > -1)) {
//    getData = 1;
    if (index1 > -1) {
      index2 = buffer.indexOf("=",index1);
      
      var = buffer.substring((index1+1), (index2));
      
      index1 = buffer.indexOf("&", index2);
      
      if (index1 == -1) {
        index1 = buffer.indexOf(" ", index2);
        endOfLine = 1;
        //index1 = bufferIndex;
      }
      
      val = buffer.substring((index2+1), (index1));
//      if (DEBUG) {
//        Serial.println(var);
//        Serial.println(val);
//      }

      val.toCharArray(valBuffer, VAL_BUF_SIZE);;
      
      if (var.equals("openm")) {
        intVal = atoi(valBuffer);
        openMin = intVal;
      } else if (var.equals("openh")) {
        intVal = atoi(valBuffer);
        openHr = intVal;
      } else if (var.equals("closem")) {
        intVal = atoi(valBuffer);
        closeMin = intVal;
      } else if (var.equals("closeh")) {
        intVal = atoi(valBuffer);
        closeHr = intVal;
      } else if (var.equals("time")) {
        //intVal = atoi(valBuffer);
        //sendStatus(String(val.toInt()));
        //RTC.set((time_t)val.toInt());
      } else if (var.equals("y")) {
        intVal = atoi(valBuffer);
        
        //sendStatus(String(intVal));
        tm.Year = CalendarYrToTm(intVal);
      } else if (var.equals("m")) {
        intVal = atoi(valBuffer);
        tm.Month = intVal;
      } else if (var.equals("d")) {
        intVal = atoi(valBuffer);
        tm.Day = intVal;
      } else if (var.equals("hr")) {
        intVal = atoi(valBuffer);
        tm.Hour = intVal;
      } else if (var.equals("min")) {
        intVal = atoi(valBuffer);
        tm.Minute = intVal;
      } else if (var.equals("sec")) {
        intVal = atoi(valBuffer);
        tm.Second = intVal;
      }
      
      

    }
  }
  //sendStatus(String(tm.Year));
  //int yr = (int)tm.Year;
  if (tm.Year > 0) {
    sendStatus(String(tm.Hour));
    RTC.write(tm);
  }
}*/


void sendStatus(int i) {
  if (wifly.open(REMOTE_SERVER, REMOTE_PORT)) {
    //Serial.print("Connected to ");
    //Serial.println(site);

    // Send the request
    wifly << F("GET /stat?") << String(i) << F(" HTTP/1.1\r\n");
    wifly << F("HOST: ") << REMOTE_SERVER << endl << endl;
  } else {
    //Serial.println("Failed to connect");
  }
  delay(10);
  wifly.close();
}
  
//  if (pos < 5) {
//    doorLocation = LOC_CLOSED;
//  } else if (pos > (maxPos-50)) {
//    doorLocation = LOC_OPEN;
//  }


