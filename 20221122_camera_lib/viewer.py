#!/usr/bin/env python3

import serial
import struct
import threading
import math

from PyQt5 import QtWidgets, QtGui, QtCore

import signal

signal.signal(signal.SIGINT, signal.SIG_DFL)

cur_pixmap = None

RES_X = 640
RES_Y = 480
SCALING = 1

TAG_SIZE_CM = 15
CAM_COEF = 1.25

def process_img(port):
    global cur_pixmap, RES_X, RES_Y, SCALING
    meta_raw = port.read(4*3)

    width, height, size = struct.unpack("<III", meta_raw)
    #print(width, height, size)
    greyscale = port.read(size)

    RES_X = width
    RES_Y = height
    SCALING = 1024/RES_X

    print(f" img size: {size} orig greyscale {width*height}")

    img = QtGui.QImage.fromData(greyscale, "JPG")
    #img = QtGui.QImage(greyscale, width, height, QtGui.QImage.Format.Format_Grayscale8)
    img = img.scaled(1024, 768)
    cur_pixmap = QtGui.QPixmap.fromImage(img)
    label.setPixmap(cur_pixmap)


def process_time(port):
    time_raw = port.read(5*4)
    detect_ms, cam_ms, heap_min, spiram_min, fps = struct.unpack("<IIIIf", time_raw)

    print(f"{detect_ms} ms, {cam_ms} ms, {heap_min/1024}, {spiram_min/1024}, {fps} fps")

    if cur_pixmap is None:
        return

    painter = QtGui.QPainter(cur_pixmap)
    red = QtGui.QPen((QtGui.QColor(255,0,0)),1);
    font = painter.font()
    font.setPointSizeF(40)
    painter.setPen(red)
    painter.setFont(font)
    painter.drawText(30, 50, f"{detect_ms}ms")

    label.setPixmap(cur_pixmap)

def points_distance(p1, p2):
    return math.sqrt(math.fabs(math.pow(p2[0] - p1[0], 2)+math.pow(p2[1] - p1[1], 2)))

def process_dets(port):
    det_raw = port.read(1+1+4+2*4)
    id, hamming, margin, x, y = struct.unpack("<BBfff", det_raw)

    print(f"    ID {id} at {x}x{y}, hamming {hamming}, margin {margin}")

    corners = []
    for _ in range(4):
        c_raw = port.read(2*4)
        cx, cy = struct.unpack("<ff", c_raw)
        corners.append([ cx, cy ])

    side_px = 0
    for i in range(4):
        p1 = corners[i]
        p2 = corners[(i+1)%4]
        print("  ", points_distance(p1, p2))
        side_px += points_distance(p1, p2)
    side_px = side_px / 4


    dist_cm = (CAM_COEF*((RES_X+RES_Y)/2)*TAG_SIZE_CM)/side_px

    print(f"Distance: {side_px} {dist_cm} cm")

    h_mat_raw = port.read(9*4)
    h_mat = struct.unpack("<fffffffff", h_mat_raw)
    print(h_mat)

    if cur_pixmap is None:
        return

    painter = QtGui.QPainter(cur_pixmap)
    blue = QtGui.QPen((QtGui.QColor(0,0,255)),1);
    font = painter.font()
    font.setPointSizeF(40)
    painter.setPen(blue)
    painter.setFont(font)
    painter.drawText(int(x*SCALING), int(y*SCALING), str(id))

    red = QtGui.QBrush((QtGui.QColor(255,0,0)),1);
    painter.setBrush(red)
    for cx, cy in corners:
        painter.drawEllipse(QtCore.QPoint(int(cx*SCALING), int(cy*SCALING)), 20, 20)

    yellow = QtGui.QPen((QtGui.QColor(255,255,0)),4)
    painter.setPen(yellow)
    for i in range(len(corners)):
        cx, cy = corners[i]
        nx, ny = corners[i+1] if i+1 < len(corners) else corners[0]

        painter.drawLine(int(cx*SCALING), int(cy*SCALING), int(nx*SCALING), int(ny*SCALING))

    label.setPixmap(cur_pixmap)

def rx_thread():
    with serial.Serial('/dev/ttyUSB0', baudrate=921600) as port:
        magic = [ 0xFF, 0x10, 0x00, 0x80 ]
        idx = 0
        while True:
            b = port.read(1)
            if b[0] == magic[idx]:
                idx += 1
                if idx >= len(magic):
                    cmd = port.read(1)[0]
                    if cmd == 1:
                        process_img(port)
                    elif cmd == 2:
                        process_time(port)
                    elif cmd == 3:
                        process_dets(port)
                    idx = 0
            else:
                idx = 0


if __name__ == "__main__":
    threading.Thread(daemon=True, target=rx_thread).start()

    app = QtWidgets.QApplication([])

    label = QtWidgets.QLabel()
    label.resize(1024, 768)
    label.show()

    app.exec()

