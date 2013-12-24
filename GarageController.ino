
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

int lastClick0 = 0;
int lastClick1 = 0;
int lastClick2 = 0;

//Settings
int minPos = 0;
int maxPos = 1500;

int crackPos = 110;

int overshootUp = 50;
int overshootDown = 50;


#define DEFAULT_OPEN_HR 8
#define DEFAULT_OPEN_MIN 0
#define DEFAULT_CLOSE_HR 18
#define DEFAULT_CLOSE_MIN 0



byte openHr = DEFAULT_OPEN_HR;
byte openMin = DEFAULT_OPEN_MIN;
byte closeHr = DEFAULT_CLOSE_HR;
byte closeMin = DEFAULT_CLOSE_MIN;

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

int doorDirection = DIR_UNK;


Adafruit_MCP23017 mcp;


//WiFly
WiFly wifly;


void setup()
{
  delay(1000);
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
  
  Serial.begin(115200); Serial.println("Garage Mk2 V2-----------------------------");
//AdaEncoder encoderA = AdaEncoder('a', ENCA_a, ENCA_b);
  //Time 
  setSyncProvider(RTC.get);

  //Expander Board
  mcp.begin(0);
  
  mcp.pinMode(DOOR_CONTROL, OUTPUT);
  mcp.digitalWrite(DOOR_CONTROL, LOW);
  Serial.print(hour());
  Serial.print(':');
  Serial.println(minute());
  
  closeDoor();
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
      case 'f':
        Serial.println("Force Cracked");
        doorLocation = LOC_CRACKED;
        break;
    }
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
    Serial.println("Door Moved");
    //doorLocation = LOC_UNK;
  }
  //}
  
  if ((millis() % 1000) == 0) {
    //updateEncoder();
    Serial.println(pos);
    
    //doorLocation = LOC_UNK;

  }
}

void openDoor() {
  if (doorLocation == LOC_OPEN) {
    return;
  }
  Serial.println("Opening Door");
  doorDestination = DEST_OPEN;
  moveDoor();
}

void closeDoor() {
  if (doorLocation == LOC_CLOSED) {
    return;
  }
  Serial.println("Closing Door");
  doorDestination = DEST_CLOSE;
  moveDoor();
}

void crackDoor() {
  if (doorLocation == LOC_CRACKED) {
    return;
  }
  closeDoor();
  Serial.println("Cracking Door");
  doorDestination = DEST_CRACK;
  moveDoor();
}

void moveDoor() {
  updateEncoder();
  if (doorDirection == DIR_UNK) {
    //Not moving. Start moving.
    pressDoorButton();
    delay(100);
  }
  
  while (1) {
    updateEncoder();
    
    switch (doorDestination) {
      case DEST_CLOSE:
        if (doorDirection == DIR_UP) {
          //Switch Dir to down
          Serial.println("Revsering Door");
          pressDoorButton();
          delay(500);
          updateEncoder();
          pressDoorButton();
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
            Serial.println("Door is closed");
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
          Serial.println("Reversing Door");
          pressDoorButton();
          delay(500);
          pressDoorButton();
        } else if (doorDirection == DIR_UNK) {
          //Stopped
          //pos = 0;
          
          //We are probablly closed? Need to check for bound.
          delay(1000);
          updateEncoder();
          if (doorDirection == DIR_UNK) {
            //Yup, we seem to have stopped.
            Serial.println("Door is open");
            doorLocation = LOC_OPEN;
            return;
          }
        }
        break;
      case DEST_CRACK:
        //if (doorDirection == DIR_UP) {
          if (pos >= (crackPos - overshootUp)) {
            pressDoorButton();
            delay(250);
            Serial.println("Door is cracked headed up.");
            updateEncoder();
            updateEncoder();
            doorLocation = LOC_CRACKED;
            return;
          }
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
  Serial.println("Button Press");
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
  
  lastClick2 = lastClick1;
  lastClick1 = lastClick0;
  lastClick0 = clicks;
  
  int sumClicks = lastClick0 + lastClick1 + lastClick2;

  if (sumClicks > 0) {
    doorDirection = DIR_UP;
  } else if (clicks < 0) {
    doorDirection = DIR_DOWN;
  } else {
    doorDirection = DIR_UNK;
  }
}
  
//  if (pos < 5) {
//    doorLocation = LOC_CLOSED;
//  } else if (pos > (maxPos-50)) {
//    doorLocation = LOC_OPEN;
//  }


