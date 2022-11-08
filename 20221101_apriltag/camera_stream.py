import cv2
import serial
import threading
import time
import sys


WIDTH = 176
HEIGHT = 144


def serial_rx(port: serial.Serial):
    while True:
        data = port.read_all()
        if data is None:
            time.sleep(0.1)
            continue
        sys.stdout.write(data.decode("utf-8", errors="replace"))
        sys.stdout.flush()


cv2.namedWindow("preview")
vc = cv2.VideoCapture(0)

vc.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
vc.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)

if vc.isOpened(): # try to get the first frame
    rval, frame = vc.read()
else:
    rval = False

with serial.Serial('/dev/ttyUSB0', baudrate=921600) as port:
    threading.Thread(daemon=True, target=serial_rx, args=(port, )).start()

    while rval:
        rval, frame = vc.read()

        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        cv2.imshow("preview", frame)

        #frame = frame.transpose()
        print(frame.shape)
        print(len(frame.tobytes()))

        port.write(b"\xFF\x10\x00\x80")
        port.write(frame.tobytes())

        [ 0xFF, 0x10, 0x00, 0x80 ]

        key = cv2.waitKey(20)
        if key == 27: # exit on ESC
            break

vc.release()
cv2.destroyWindow("preview")
