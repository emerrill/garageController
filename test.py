#!/usr/bin/python
# -*- coding: <<encoding>> -*-
#-------------------------------------------------------------------------------
#   <<project>>
# 
#-------------------------------------------------------------------------------



import sys, time
import usb # 1.0 not 0.4

sys.path.append("..")

from arduino.usbdevice import ArduinoUsbDevice



def checkForData():
    global theDevice
    try:
        charRead = chr(theDevice.read())
        print charRead
        time.sleep(0.001)
    except:
        # TODO: Check for exception properly
        time.sleep(0.5)



global theDevice


try:
    theDevice = ArduinoUsbDevice(idVendor=0x16c0, idProduct=0x05df)       
except:
    # TODO: Check for exception properly
    print 'No DigiUSB Device Detected'
    exit();
time.sleep(1)
theDevice.write(ord('a'))

while 1:
    checkForData()