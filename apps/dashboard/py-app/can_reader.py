import socket
import threading
import math
import time
from enum import IntEnum

class SourceAddress(IntEnum):
    CORE         = 0x00
    ENGINE       = 0x01
    TRANSMISSION = 0x03
    DASHBOARD    = 0x17
    SIMULATOR    = 0xE5

class CANReader:
    def __init__(self, signal, use_demo):
        self.signal = signal
        self.use_demo = use_demo

        if use_demo:
            threading.Thread(target=self.demo_loop, daemon=True).start()
        else:
            try:
                self.sock = socket.socket(socket.AF_CAN, socket.SOCK_DGRAM, socket.CAN_J1939)
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                self.sock.bind(("vcan0", 0, socket.J1939_NO_PGN, SourceAddress.DASHBOARD))
                print("J1939 initialized:", self.sock.getsockname())
            except OSError as e:
                print("J1939 initialization failed:", e)

            threading.Thread(target=self.read_loop, daemon=True).start()

    def demo_loop(self):
        t = 0
        while True:
            spd  = int((math.sin(t) + 1) * 100)
            rpm  = int((math.sin(t + 1) + 1) * 2000)
            fuel = int((math.sin(t + 2) + 1) * 50)
            temp = int((math.sin(t + 3) + 1) * 75)
            warn = math.sin(t + 4) > 0.7

            self.signal.emit(spd, rpm, fuel, temp, warn)

            t += 0.05
            time.sleep(0.05)

    def read_loop(self):
        spd = rpm = fuel = temp = 0
        warn = False

        while True:
            data, addr = self.sock.recvfrom(1785)

            # J1939 sockaddr normally contains:
            # interface, name, pgn, address
            ifname, name, pgn, source_addr = addr

            if pgn == 0xFEF2:
                if len(data) >= 2:
                    spd = int.from_bytes(data[0:2], 'little')

            elif pgn == 0xF004:
                if len(data) >= 2:
                    rpm = int.from_bytes(data[0:2], 'little')

            elif pgn == 0xFEFC:
                if len(data) >= 1:
                    fuel = data[0]

            elif pgn == 0xFEEE:
                if len(data) >= 1:
                    temp = data[0]

            elif pgn == 0xFECA:
                if len(data) >= 1:
                    warn = (data[0] & 0x10) != 0

            self.signal.emit(spd, rpm, fuel, temp, warn)
