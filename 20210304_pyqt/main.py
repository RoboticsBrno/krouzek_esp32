#!/usr/bin/env python3

import sys

# 1. Import `QApplication` and all the required widgets
from PyQt5.QtWidgets import QApplication
from PyQt5.QtWidgets import QLabel, QPushButton
from PyQt5.QtWidgets import QWidget

def btn_clicked():
    print("Clicked!")

if __name__ == "__main__":
    # 2. Create an instance of QApplication
    app = QApplication(sys.argv)

    # 3. Create an instance of your application's GUI
    window = QWidget()
    window.setWindowTitle('PyQt5 App')
    window.setGeometry(100, 100, 280, 180)
    window.move(60, 15)
    
    helloMsg = QLabel('<h1>Hello World!</h1>', parent=window)
    helloMsg.move(60, 15)

    # button
    btn = QPushButton("Tlačítko!!", parent=window)
    btn.move(60, 70)
    btn.clicked.connect(btn_clicked)

    window.show()

    # 5. Run your application's event loop (or main loop)
    sys.exit(app.exec_())
