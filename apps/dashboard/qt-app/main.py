from PySide6.QtWidgets import QWidget, QApplication, QGridLayout
from PySide6.QtCore import Signal
from widgets.gauge_widget import GaugeWidget
from can_reader import CANReader
from widgets.warning_widget import WarningWidget
import sys

class Dashboard(QWidget):
    values_signal = Signal(int, int, int, int, bool)

    def __init__(self, use_demo):
        super().__init__()
        self.use_demo = use_demo


        layout = QGridLayout(self)

        # gauges
        self.speed   = GaugeWidget("Speed",   0, 200, -130, 130)
        self.rpm     = GaugeWidget("RPM",     0, 4000, -130, 130)
        self.fuel    = GaugeWidget("Fuel",    0, 100, -130, 130)
        self.coolant = GaugeWidget("Coolant", 0, 150, -130, 130)

        # warning
        self.warning = WarningWidget("gauges/warning.svg")


        # layout
        layout.addWidget(self.speed,   0, 0)
        layout.addWidget(self.rpm,     0, 1)
        layout.addWidget(self.fuel,    1, 0)
        layout.addWidget(self.coolant, 1, 1)
        layout.addWidget(self.warning, 2, 0, 1, 2)

        # connect signal
        self.values_signal.connect(self.update_values)

        # start demo CAN reader
        self.reader = CANReader(self.values_signal, self.use_demo)

        self.resize(900, 600)

    def update_values(self, spd, rpm, fuel, temp, warn):
        self.speed.set_value(spd)
        self.rpm.set_value(rpm)
        self.fuel.set_value(fuel)
        self.coolant.set_value(temp)
        self.warning.set_state(warn)

if __name__ == "__main__":
    # Default: demo = False
    use_demo = False

    # Command-line argument parsing
    # Examples:
    #   python3 main.py demo
    #   python3 main.py PLC
    if len(sys.argv) > 1:
        arg = sys.argv[1].strip().lower()
        if arg == "demo":
            use_demo = True
        elif arg == "plc":
            use_demo = False
        else:
            print(f"Unknown mode '{arg}', expected 'demo' or 'PLC'.")

    app = QApplication([])
    dash = Dashboard(use_demo)
    dash.show()
    app.exec()
