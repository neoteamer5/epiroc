import can
import threading
import math
import time

class CANReader:
    def __init__(self, signal, use_demo):
        self.signal = signal
        self.use_demo = use_demo

        if use_demo:
            threading.Thread(target=self.demo_loop, daemon=True).start()
        else:
            self.bus = can.Bus(interface='socketcan', channel='vcan0', bitrate=250000)
            threading.Thread(target=self.read_loop, daemon=True).start()

    def demo_loop(self):
        t = 0
        while True:
            spd  = int((math.sin(t) + 1) * 100)
            rpm  = int((math.sin(t+1) + 1) * 2000)
            fuel = int((math.sin(t+2) + 1) * 50)
            temp = int((math.sin(t+3) + 1) * 75)
            warn = (math.sin(t+4) > 0.7)

            self.signal.emit(spd, rpm, fuel, temp, warn)

            t += 0.05
            time.sleep(0.05)

    def read_loop(self):
        spd = rpm = fuel = temp = 0
        warn = False

        while True:
            msg = self.bus.recv()
            pgn = (msg.arbitration_id >> 8) & 0xFFFF

            if pgn == 0xFEF2:
                spd = int.from_bytes(msg.data[0:2], 'little')
            elif pgn == 0xF004:
                rpm = int.from_bytes(msg.data[0:2], 'little')
            elif pgn == 0xFEFC:
                fuel = msg.data[0]
            elif pgn == 0xFEEE:
                temp = msg.data[0]
            elif pgn == 0xFECA:
                warn = (msg.data[0] & 0x10) != 0

            self.signal.emit(spd, rpm, fuel, temp, warn)
