import time
import serial

print("opening serial connection")
ser = serial.Serial('COM8', 9600)
print("serial connection open")

data = b'1023|900|800|700|600|500\r\n'

for i in range(0,5):
    ser.write(b'deej.core.values\r\n')
    line = ser.readline()
    print(line)

start = False

for i in range(0,10):
    #ser.write(b'deej.core.values\n')
    if not start:
        start = True
        #ser.write(b'deej.core.start\r\n')
    #ser.write(b'deej.core.values.HR\r\n')
    line = ser.readline()
    print(line)
    ser.write(b'deej.core.receive\r\n')
    ser.write(data)
    time.sleep(0.1)
ser.close()


