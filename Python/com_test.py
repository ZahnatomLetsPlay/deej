import serial
import time

ser = serial.Serial('COM6',9600)

data = b'0|0|0|0|0|1023\n'
send = False

while True:
    line = ser.readline()
    print(line)
    if b'receive' in line:
        data = ser.readline()
        print(data)
    if b'start' in line:
        send = True
    if b'stop' in line:
        send = False
    if b'values' in line:
        print("sending", data)
        ser.write(data)
    if send:
        print("sending", data)
        ser.write(data)
    print("------------")
    print(data)
    print("------------")
    time.sleep(0.1)
