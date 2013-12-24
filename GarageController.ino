
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


//Settings
int minPos = 0;
int maxPos = 1500;

int crackPos = 200;

int overshootUp = 45;
int overshootDown = 45;

#define DEFAULT_OPEN_HR 8
#define DEFAULT_CLOSE_HR 19



byte openHr = DEFAULT_OPEN_HR;
byte closeHr = DEFAULT_CLOSE_HR;

int pos = 0;
int lastDir = 0;

#define DOOR_CONTROL 1

#define DEST_NONE 0
#define DEST_CLOSE 1
#define DEST_OPEN 2
#define DEST_CRACK 3

int doorDestination = DEST_NONE;

#define LOC_UNK 0
#define LOC_CLOSED 1
#define LOC_OPEN 2
#define LOC_CRACKED 3
#define LOC_MOVING 4

int doorLocation = LOC_UNK;

#define DIR_UNK 0
#define DIR_UP 1
#define DIR_DOWN 2

int lastDoorTravelDir = DIR_UNK;


Adafruit_MCP23017 mcp;


//WiFly
WiFly wifly;


void setup()
{
  //Serial.begin(9600);
  //wifly.begin(&Serial, NULL)
//  if (!wifly.isAssociated()) {
//	/* Setup the WiFly to connect to a wifi network */
//	Serial.println("Joining network");
//	wifly.setSSID(mySSID);
//	wifly.setPassphrase(myPassword);
//	wifly.enableDHCP();
//
//	if (wifly.join()) {
//	    Serial.println("Joined wifi network");
//	} else {
//	    Serial.println("Failed to join wifi network");
//	    terminal();
//	}
//    } else {
//        Serial.println("Already joined network");
//    }
//    if (wifly.isConnected()) {
//        Serial.println("Old connection active. Closing");
//	wifly.close();
//    }
  
  Serial.begin(115200); Serial.println("---------------------------------------");
//AdaEncoder encoderA = AdaEncoder('a', ENCA_a, ENCA_b);
  //Time 
  setSyncProvider(RTC.get);

  //Expander Board
  mcp.begin(0);
  
  mcp.pinMode(DOOR_CONTROL, OUTPUT);
  mcp.digitalWrite(DOOR_CONTROL, LOW);
  
  Serial.println(hour());
}

void loop() 
{
  while (Serial.available()) {
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
    }
  }
  
  
  
  
  
  // Just do this 'occasionally'
  if ((millis() % 50) == 0) {
    //updateEncoder();
    
    if (minute() == 0 && second() == 0) {
      if (hour() == openHr) {
        crackDoor();
        delay(1000);
      } else if (hour() == closeHr) {
        closeDoor();
        delay(1000);
      }
    }
  }
  
  if ((millis() % 1000) == 0) {
    updateEncoder();
    Serial.println(pos);
    delay(1);
  }
}

void openDoor() {
  Serial.println("Opening Door");
  if (doorLocation == LOC_OPEN) {
    return;
  }
  
  pressDoorButton();
  
  delay(250);
  
  unsigned long startTime = millis();
  while ((millis() - startTime) < 60000) {
    updateEncoder();
    if (doorLocation == LOC_OPEN) {
      Serial.println("Door Opened");
      return;
    } else if (doorLocation == LOC_CLOSED) {
      Serial.println("Door hit low, reversing");
      pressDoorButton();
      delay(1000);
    }
  }
  
  //Fail Closed
  Serial.println("Timeout");
  closeDoor();
}

void closeDoor() {
  Serial.println("Closing Door");
  if (doorLocation == LOC_CLOSED) {
    return;
  }
  
  pressDoorButton();
  
  delay(250);
  
  unsigned long startTime = millis();
  while ((millis() - startTime) < 60000) {
    updateEncoder();
    if (doorLocation == LOC_CLOSED) {
      Serial.println("Door Closed");
      return;
    } else if (doorLocation == LOC_OPEN) {
      Serial.println("Door hit high, reversing");
      pressDoorButton();
      delay(1000);
    }
  }
  
  //Fail Closed
  Serial.println("Timeout");
  closeDoor();
}

void crackDoor() {
  if (doorLocation == LOC_CRACKED) {
    return;
  }
  
  pressDoorButton();
  
  delay(250);
  
  unsigned long startTime = millis();
  while ((millis() - startTime) < 60000) {
    updateEncoder();
    
    if (lastDir == DIR_DOWN) {
      if (pos <= (crackPos + overshootDown)) {
        pressDoorButton();
        doorLocation = LOC_CRACKED;
        return;
      }
    } else {
      if (pos >= (crackPos - overshootUp)) {
        pressDoorButton();
        doorLocation = LOC_CRACKED;
        return;
      }
    }
  }
  
}

void pressDoorButton() {
  doorLocation = LOC_MOVING;
  mcp.digitalWrite(DOOR_CONTROL, HIGH);
  delay(200);
  mcp.digitalWrite(DOOR_CONTROL, LOW);
  
}

void updateEncoder() {
  AdaEncoder *thisEncoder;
  thisEncoder=AdaEncoder::genie();
  if (thisEncoder != NULL) {
    int clicks = thisEncoder->clicks;
    thisEncoder->clicks = 0;
    pos += clicks;
    if (pos < 0) {
      pos = 0; //0 is min, door closed
    }
    if (pos > maxPos) {
      maxPos = pos;
    }
    
    if (clicks > 0) {
      lastDir = DIR_UP;
    } else if (clicks < 0) {
      lastDir = DIR_DOWN;
    } else {
      lastDir = DIR_UNK;
    }
  }
  
  if (pos < 5) {
      doorLocation = LOC_CLOSED;
    } else if (pos > (maxPos-50)) {
      doorLocation = LOC_OPEN;
    }
}

