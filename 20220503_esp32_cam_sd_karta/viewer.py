#!/usr/bin/env python3

import serial
import struct
import threading

from PyQt5 import QtWidgets, QtGui

def process_jpg(port):
    global label
    meta_raw = port.read(4*3)
    width, height, size = struct.unpack("<III", meta_raw)
    print(width, height, size)
    jpg = port.read(size)

    grey_raw = port.read(4*3)
    grey_size, grey_w, grey_h  = struct.unpack("<III", grey_raw)
    greyscale = port.read(grey_size)

    print("greyscale size %d %d %d" % (grey_size, grey_w, grey_h))

    durations_raw = port.read(2*4 + 1)
    phash_distance, uart, sd = struct.unpack("<BII", durations_raw)
    print("Uart took %dms, sd took %dms, phash distance %d" % (uart, sd, phash_distance))

    img = QtGui.QImage.fromData(jpg, "JPG").scaledToWidth(640)
    pixmap = QtGui.QPixmap.fromImage(img)
    label.setPixmap(pixmap)

    grey_img = QtGui.QImage(greyscale, 38, 38, QtGui.QImage.Format.Format_RGB888).scaled(320, 320)
    pixmap = QtGui.QPixmap.fromImage(grey_img)
    grey_window.setPixmap(pixmap)


def jpg_thread():
    with serial.Serial('/dev/ttyUSB0', baudrate=115200) as port:
        magic = [ 0xFF, 0x10, 0x00, 0x80 ]
        idx = 0
        while True:
            b = port.read()
            if b[0] == magic[idx]:
                idx += 1
                if idx >= len(magic):
                    process_jpg(port)
                    idx = 0
            else:
                idx = 0


if __name__ == "__main__":
    threading.Thread(daemon=True, target=jpg_thread).start()

    app = QtWidgets.QApplication([])

    button = QtWidgets.QPushButton("Click to Exit")
    button.setWindowTitle("Goodbye World")
    button.clicked.connect(app.quit)

    label = QtWidgets.QLabel()
    label.resize(640, 480)
    label.show()

    grey_window = QtWidgets.QLabel()
    grey_window.resize(320, 320)
    grey_window.show()

    button.show()

    app.exec()

