import sys
from PySide6.QtWidgets import QApplication, QWidget, QGridLayout
from can_reader import CANReader
from widgets.gauge_widget import GaugeWidget
from widgets.warning_widget import WarningWidget

class Dashboard(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("J1939 Dashboard")
        layout = QGridLayout()

        self.speed = GaugeWidget("Speed", 0, 200, -130, 130)
        self.rpm   = GaugeWidget("RPM",   0, 4000, -130, 130)
        self.fuel  = GaugeWidget("Fuel",  0, 100, -130, 130)
        self.coolant = GaugeWidget("Coolant", 0, 150, -130, 130)

        self.warning = WarningWidget("gauges/warning.svg")

        layout.addWidget(self.speed, 0, 0)
        layout.addWidget(self.rpm, 0, 1)
        layout.addWidget(self.fuel, 1, 0)
        layout.addWidget(self.coolant, 1, 1)
        layout.addWidget(self.warning, 2, 0, 1, 2)

        self.setLayout(layout)

        self.can = CANReader(self.update_values)

    def update_values(self, spd, rpm, fuel, temp, warn):
        self.speed.set_value(spd)
        self.rpm.set_value(rpm)
        self.fuel.set_value(fuel)
        self.coolant.set_value(temp)
        self.warning.set_state(warn)

app = QApplication(sys.argv)
dash = Dashboard()
dash.show()
app.exec()
