import time
import threading
import math
from PySide6.QtCore import QMetaObject, Qt

class CANReader:
    def __init__(self, callback):
        self.callback = callback
        threading.Thread(target=self.demo_loop, daemon=True).start()

    def demo_loop(self):
        t = 0
        while True:
            spd  = int((math.sin(t) + 1) * 100)
            rpm  = int((math.sin(t + 1) + 1) * 2000)
            fuel = int((math.sin(t + 2) + 1) * 50)
            temp = int((math.sin(t + 3) + 1) * 75)
            warn = (math.sin(t + 4) > 0.7)

            QMetaObject.invokeMethod(
                self.callback,
                "__call__",
                Qt.QueuedConnection,
                spd, rpm, fuel, temp, warn
            )

            t += 0.05
            time.sleep(0.05)
