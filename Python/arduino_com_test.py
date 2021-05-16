import time
import serial
import random

print("opening serial connection")
ser = serial.Serial('COM8', 9600)
print("serial connection open")

data = "651|987|169|189|486|007\r\n"
dataold = ""
ms = time.time()*1000

for i in range(0,5):
    ser.write(b'deej.core.values\r\n')
    line = ser.readline()
    print(line)

start = False

for i in range(0,100):
    #ser.write(b'deej.core.values\n')
    if not start:
        start = True
        #ser.write(b'deej.core.start\r\n')
    #print("commanding to give values")
    ser.write(b'deej.core.values\r\n')
    #print("commanded to give values, reading...")
    line = ser.readline()
    print(line)
    #ser.write(b'deej.core.receive\r\n')
    #ser.write(bytes(data, "ASCII"))
    ser.write(bytes("deej.core.receive\r\n" + data, "ASCII"))
    line = ser.readline()
    print(line, "correct:", (line == bytes(dataold, "ASCII")))
    dataold = data + ""
    data = "" + str(random.randint(0,1023)) + "|"
    data = data + str(random.randint(0,1023)) + "|"
    data = data + str(random.randint(0,1023)) + "|"
    data = data + str(random.randint(0,1023)) + "|"
    data = data + str(random.randint(0,1023)) + "|"
    data = data + str(random.randint(0,1023)) + "\r\n"
    #time.sleep(0.1)
ser.close()
ms = time.time()*1000-ms
print(ms)


