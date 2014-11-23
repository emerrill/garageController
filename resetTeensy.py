
import serial
import serial.tools.list_ports

port = list(serial.tools.list_ports.comports())[0]
portdev = port[0]

ser = serial.Serial(portdev, 115200)

ser.write("r")