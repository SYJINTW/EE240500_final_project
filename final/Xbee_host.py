import serial
import time
import sys,tty,termios

# XBee setting
serdev = '/dev/ttyUSB0'
s = serial.Serial(serdev, 9600, timeout=3)

print('Start Communication, sending RPC')
send = 'start'
s.write("/xbee_start/run\n".encode())
while send != 'done':
    send = s.readline()
    print(send)
    if (send == b'driver'):
        # send to remote
        s.write("/driver/run\n".encode())

    elif (send == b'color'):
        # send to remote
        s.write("/color_det/run\n".encode())

    elif (send == b'line'):
        # send to remote
        s.write("/line_det/run\n".encode())

    elif (send == b'circle'):
        # send to remote
        s.write("/circle/run\n".encode())

    elif (send == b'parking'):
        # send to remote
        s.write("/parking/run 5 5 w\n".encode())
    
    elif (send == b'stop'):
        # send to remote
        s.write("/stop/run\n".encode())


s.close()