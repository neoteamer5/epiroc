from PySide6.QtWidgets import QWidget, QApplication, QGridLayout
from PySide6.QtCore import Signal
from widgets.gauge_widget import GaugeWidget
from widgets.warning_widget import WarningWidget
from PySide6.QtCore import QTimer


import ctypes

lib = ctypes.CDLL("./build/libcan_reader.so")

class CANData(ctypes.Structure):
    _fields_ = [
        ("spd", ctypes.c_int),
        ("rpm", ctypes.c_int),
        ("fuel", ctypes.c_int),
        ("temp", ctypes.c_int),
        ("warn", ctypes.c_int),
    ]

lib.start_can_reader_real()


class Dashboard(QWidget):
    values_signal = Signal(int, int, int, int, bool)

    def __init__(self, use_demo):
        super().__init__()
        self.use_demo = use_demo

        layout = QGridLayout(self)

        self.speed   = GaugeWidget("Speed",   0, 200, -130, 130)
        self.rpm     = GaugeWidget("RPM",     0, 4000, -130, 130)
        self.fuel    = GaugeWidget("Fuel",    0, 100, -130, 130)
        self.coolant = GaugeWidget("Coolant", 0, 150, -130, 130)
        self.warning = WarningWidget("gauges/warning.svg")

        layout.addWidget(self.speed,   0, 0)
        layout.addWidget(self.rpm,     0, 1)
        layout.addWidget(self.fuel,    1, 0)
        layout.addWidget(self.coolant, 1, 1)
        layout.addWidget(self.warning, 2, 0, 1, 2)

        self.values_signal.connect(self.update_values)

        # NEW: poll C++ CAN reader
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.poll_can_reader)
        self.timer.start(50)

        self.resize(900, 600)

    def poll_can_reader(self):
        data = CANData()
        lib.get_can_data(ctypes.byref(data))

        self.values_signal.emit(
            data.spd,
            data.rpm,
            data.fuel,
            data.temp,
            bool(data.warn)
        )

    def update_values(self, spd, rpm, fuel, temp, warn):
        self.speed.set_value(spd)
        self.rpm.set_value(rpm)
        self.fuel.set_value(fuel)
        self.coolant.set_value(temp)
        self.warning.set_state(warn)

if __name__ == "__main__":
    mode = input("Use demo data? (y/n): ").strip().lower()
    use_demo = (mode == "y")

    app = QApplication([])
    dash = Dashboard(use_demo)
    dash.show()
    app.exec()

