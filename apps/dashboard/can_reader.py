import can
from threading import Thread
import time

class CANReader:
    def __init__(self, callback):
        self.callback = callback
        self.bus = can.interface.Bus("vcan0", bustype="socketcan")
        Thread(target=self.loop, daemon=True).start()

    def loop(self):
        while True:
            msg = self.bus.recv(timeout=0.01)
            if msg:
                spd = msg.data[0]
                rpm = msg.data[1] * 32
                fuel = msg.data[2]
                temp = msg.data[3]
                warn = msg.data[4] > 0
                self.callback(spd, rpm, fuel, temp, warn)
            time.sleep(0.01)
