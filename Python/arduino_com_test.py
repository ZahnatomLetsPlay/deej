import time
import serial

print("opening serial connection")
ser = serial.Serial('COM8', 9600)
print("serial connection open")

data = b'1023|1022|1021|1020|1019|1018\r\n'

for i in range(0,5):
    ser.write(b'deej.core.values\r\n')
    line = ser.readline()
    print(line)

start = False

for i in range(0,3):
    #ser.write(b'deej.core.values\n')
    if not start:
        start = True
        #ser.write(b'deej.core.start\r\n')
    print("commanding to give values")
    ser.write(b'deej.core.values\r\n')
    print("commanded to give values, reading...")
    line = ser.readline()
    print(line)
    time.sleep(0.1)
    ser.write(b'deej.core.receive\r\n')
    ser.write(data)
    line = ser.readline()
    print(line)
    #time.sleep(0.1)
ser.close()


