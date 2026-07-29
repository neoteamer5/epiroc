import time
import threading
import math

class CANReader:
    def __init__(self, signal):
        self.signal = signal
        threading.Thread(target=self.demo_loop, daemon=True).start()

    def demo_loop(self):
        t = 0
        while True:
            spd  = int((math.sin(t) + 1) * 100)
            rpm  = int((math.sin(t + 1) + 1) * 2000)
            fuel = int((math.sin(t + 2) + 1) * 50)
            temp = int((math.sin(t + 3) + 1) * 75)
            warn = (math.sin(t + 4) > 0.7)

            self.signal.emit(spd, rpm, fuel, temp, warn)

            t += 0.05
            time.sleep(0.05)
