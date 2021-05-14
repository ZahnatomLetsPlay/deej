import time
import serial

print("opening serial connection")
ser = serial.Serial('COM8', 115200)
print("serial connection open")

data = b'1023|900|800|700|600|500\n'

#for i in range(0,5):
#    print("sending")
#    ser.write(b'deej.core.values\n')
#    print("sent, receiving")
#    line = ser.readline()
#    print("received")
#    print("Line:", line)
start = False

for i in range(0,10):
    #ser.write(b'deej.core.values\n')
    line = ser.readline()
    print(line)
    if not start:
        start = True
        ser.write(b'deej.core.start\n')
    ser.write(b'deej.core.receive\n')
    ser.write(data)
    time.sleep(0.1)
ser.close()
