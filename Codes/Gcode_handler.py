import serial
import time

ser = serial.Serial('COM7', 115200)

file = open("g_codeMittel", "r")
print("Connected")
time.sleep(2)
ser.flushInput()
#_ = ser.readline()
for line in file:
    l = line.strip()
    ser.write(str.encode(l+"\n"))
    _ = ser.readline()

print("GCODE SENT")
time.sleep(0.5)
file.close()
ser.close()
