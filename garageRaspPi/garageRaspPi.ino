/* Encoder Library - Basic Example
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 *
 * This example code is in the public domain.
 */

#define ROT_A 5
#define ROT_B 6
#define RELAY 0
#define UPDATE_RATE 100000
#define NO_POSITION -32768
#define MOTIONLESS_CYCLES 7

#include <Encoder.h>
#include <TimerOne.h>

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder rot(ROT_A, ROT_B);
//   avoid using pins with LEDs attached

int16_t target = NO_POSITION;
int16_t oldLocation = NO_POSITION;
int16_t startLocation = NO_POSITION;
int8_t moving = 0;
int8_t pressButton = 0;
int8_t cyclesSinceMotion = 0;
int8_t lastDirection = 0;
int8_t calibrated = 0;

void setup() {
  Serial.begin(115200);
  //Serial.println("Basic Encoder Test:");
  
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  
  Timer1.initialize(UPDATE_RATE);
  Timer1.attachInterrupt(writeMovingPos);
  
  delay(2000);
  //Serial.println("Booted");
}

long oldPosition  = NO_POSITION;

void loop() {
  byte lastRead;

  if (pressButton) {
    pressButton = 0;
    buttonPress();
  }
  
  while (Serial.available()) {
    lastRead = Serial.read();
    switch (lastRead) {
      case 'B':
        // Press Button.
        buttonPress();
        oldPosition = NO_POSITION;
        break;
      case 'T':
        // Press Button with target.
        target = readInt();
        buttonPress();
        break;
      case 'G':
        // Get Count.
        Serial.println(rot.read());
        break;
      case 'g':
        // Get calibrated.
        Serial.println(calibrated);
        break;
      case 'R':
        // Restart.
        restart();
        break;
      case 'r':
        // Reset.
        reboot();
        break;
      case 'S':
        setEncoder(readInt());
        break;
      case 'C':
        clearAll();
        break;
      case 'c':
        // Set calibrated.
        calibrated = 1;
        break;
    }
  }
  
}

int16_t readInt() {
  int16_t value = 0;
  int8_t neg = 0;
  int8_t lastRead = 0;
  lastRead = (int8_t)Serial.peek();
  if (lastRead == '-') {
    neg = 1;
  }
  while (Serial.available()) {
    lastRead = (int8_t)Serial.read();
    //lastRead -= 48;d
    if ((lastRead >= 48) && (lastRead <= 57)) {
      lastRead -= 48;
      value *= 10;
      value += lastRead;
    }
  }
  if (neg) {
    value *= -1;
  }
  return value;
}

void clearAll() {
  resetEncoder();
  target = NO_POSITION;
  oldLocation = 0;
  startLocation = NO_POSITION;
  moving = 0;
  pressButton = 0;
}

void restart() {
  _restart_Teensyduino_();
}

void reboot() {
  // Bootloader
  _reboot_Teensyduino_();
}

void buttonPress() {
  if (!moving) {
    // Start Moving.
    startLocation = rot.read();
  }
  //Serial.println("Button Press");
  digitalWrite(RELAY, HIGH);
  delay(250);
  digitalWrite(RELAY, LOW);
  if (!moving) {
    // Start Moving.
    moving = 1;
  }
}

void resetEncoder() {
  setEncoder(0);
}

void setEncoder(uint16_t pos) {
  rot.write(pos);
}

void writeMovingPos() {
  if (moving) {
    int16_t loc = rot.read();
    // Check if we have a target.
    if (target != NO_POSITION) {
      // If the target is above the start.
      if (startLocation > target) {
        if (loc < target) {
          // Stop.
          target = NO_POSITION;
          pressButton = 1;
        }
      } else if (startLocation < target) {
        if (loc > target) {
          // Stop.
          target = NO_POSITION;
          pressButton = 1;
        }
      }
    }
    if (loc == oldLocation) {
      cyclesSinceMotion++;
      if (cyclesSinceMotion > MOTIONLESS_CYCLES) {
        moving = 0;
        cyclesSinceMotion = 0;
        startLocation = NO_POSITION;
        Serial.println("Done");
      }
    } else {
      cyclesSinceMotion = 0;
      // Serial.println(rot.read());
    }
    oldLocation = loc;
  }
}
