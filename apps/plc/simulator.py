import time
import math
import can

bus = can.Bus(interface='socketcan', channel='vcan0', bitrate=250000)

def send(pgn, data):
    arb_id = 0x18 << 24 | pgn << 8 | 0x80   # priority=6, SA=0x80
    msg = can.Message(arbitration_id=arb_id, data=data, is_extended_id=True)
    bus.send(msg)

while True:
    t = time.time()

    speed = int((math.sin(t) + 1) * 100)      # 0–200
    rpm   = int((math.sin(t+1) + 1) * 2000)   # 0–4000
    fuel  = int((math.sin(t+2) + 1) * 50)     # 0–100
    temp  = int((math.sin(t+3) + 1) * 75)     # 0–150
    
    warn = temp > 120

    # PGN 65266 – Speed
    send(0xFEF2, speed.to_bytes(2, 'little') + b'\xFF'*6)

    # PGN 65265 – RPM
    send(0xF004, rpm.to_bytes(2, 'little') + b'\xFF'*6)

    # PGN 65257 – Fuel
    send(0xFEFC, fuel.to_bytes(1, 'little') + b'\xFF'*7)

    # PGN 65262 – Coolant Temp
    send(0xFEEE, temp.to_bytes(1, 'little') + b'\xFF'*7)

    # PGN 65226 – Warning Lamp
    lamp = 0x10 if warn else 0x00
    send(0xFECA, bytes([lamp]) + b'\xFF'*7)

    time.sleep(0.05)
