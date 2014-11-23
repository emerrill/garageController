#!/usr/bin/python

import sys, time, datetime, serial, select, urllib, os
import ConfigParser
import serial.tools.list_ports




UP_OVERSHOOT = 50
DOWN_OVERSHOOT = 138
CRACK_POS = 400
OVERSHOOT_POS = 900
WAIT_TIMEOUT = 30


def teensyMoveDoor(target):
    ser.write("T"+str(target))

def buttonPress():
    ser.write("B")
    print "Button Press"

def readValue():
    #ser.timeout = 2
    line = ser.readline()
    #ser.timeout = 0
    return int(line)

def getValue():
    ser.flushInput()
    ser.write("G")
    return readValue()

def teensyRestart():
    ser.write("R")

def teensyClear():
    ser.write("C")

def setCal():
    ser.write("c")

def getCal():
    ser.flushInput()
    ser.write("g")
    return readValue()

def setTeensy(value):
    ser.write("S"+str(value))

def waitForDone():
    # TODO update pos
    startTime = time.time();

    while 1:
        waitTime = time.time() - startTime;
        if (waitTime > WAIT_TIMEOUT):
            print "waitForDone() Timed out"
            return
        line = ser.readline().rstrip()
        if (line == "Done"):
            return


def calibrate():
    start = getValue()
    buttonPress()
    time.sleep(0.500)
    now = getValue()
    
    # Going up. Reverse.
    if (now > start):
        time.sleep(1)
        print "Stopping"
        buttonPress()
        waitForDone()
        print "Reversing"
        buttonPress()
    
    waitForDone()
    teensyClear()
    setCal()
    print "Cal done"

def calIfNeeded():
    cal = getCal()
    if (cal == 0):
        calibrate()


def crackDoor():
    calIfNeeded()
    teensyMoveDoor(OVERSHOOT_POS - UP_OVERSHOOT)
    waitForDone()
    teensyMoveDoor(CRACK_POS + DOWN_OVERSHOOT)
    waitForDone()

def closeDoor():
    calIfNeeded()
    teensyMoveDoor(OVERSHOOT_POS - UP_OVERSHOOT)
    waitForDone()
    buttonPress()
    waitForDone()
    pos = getValue()
    if (pos < 100):
        teensyClear()
    else:
        # TODO - do stuff if it didn't close all the way...
        print "Incorrect move"


def execCommand(command):
    if (line == "open"):
        crackDoor()
    elif (line == "crack"):
        crackDoor()
    elif (line == "close"):
        closeDoor()
    elif (line == "cal"):
        calibrate()
    elif (line == "button"):
        buttonPress()
    elif (line == "get"):
        print getValue()
    elif (line == "clear"):
        teensyClear()
    elif (line == "set"):
        print setTeensy(500)
    elif (line == "getCal"):
        print getCal()
    elif (line == "setCal"):
        setCal()
    elif (line == "restart"):
        teensyRestart()
    #time.sleep(1)

# Setup time.
current_time = datetime.datetime.now().time()

# Load the port.
port = list(serial.tools.list_ports.comports())[0]
portdev = port[0]
ser = serial.Serial(portdev, 115200, timeout=2)


if len(sys.argv) > 1:
    line = sys.argv[1]
    execCommand(line)

    
else:
    ser.timeout = 0
    while 1:
        while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            line = sys.stdin.readline().rstrip()
            if line:
                execCommand(line)
            else: # an empty line means stdin has been closed
                print('eof')
                exit(0)
        else:
            line = ser.readline()
            if (line):
                print line.rstrip()


config = ConfigParser.ConfigParser()
dir = os.path.dirname(__file__)
filename = os.path.join(dir, 'credentials.ini')
config.read(filename)

urllib.urlopen("https://data.sparkfun.com/input/" + config.get("phant", "publickey") + "?private_key=" + config.get("phant", "privatekey") + "&position=" + str(getValue()))

ser.close()
print "ending"


