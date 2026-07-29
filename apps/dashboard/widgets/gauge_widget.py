from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout
from PySide6.QtGui import QPainter
from PySide6.QtCore import Qt
from PySide6.QtGui import QPen

def value_to_angle(val, vmin, vmax, amin, amax):
    if val < vmin: val = vmin
    if val > vmax: val = vmax
    ratio = (val - vmin) / (vmax - vmin)
    return amin + ratio * (amax - amin)

class GaugeWidget(QWidget):
    def __init__(self, title, min_val, max_val, start_angle, end_angle):
        super().__init__()
        self.title = title
        self.min_val = min_val
        self.max_val = max_val
        self.start_angle = start_angle
        self.end_angle = end_angle
        self.value = 0

        # Add a label for numeric value
        self.value_label = QLabel("0")
        self.value_label.setAlignment(Qt.AlignCenter)

        # Layout: gauge on top, value below
        layout = QVBoxLayout(self)
        layout.addStretch()
        layout.addWidget(self.value_label)

    def set_value(self, val):
        self.value = val
        self.value_label.setText(str(val))
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Draw gauge arc
        w = self.width()
        h = self.height()
        cx = w / 2
        cy = h / 2
        radius = min(w, h) * 0.4

        # dial circle
        painter.setPen(QPen(Qt.white, 3))
        painter.drawEllipse(cx - radius, cy - radius, 2 * radius, 2 * radius)

        # needle angle
        angle = value_to_angle(self.value, self.min_val, self.max_val, self.start_angle, self.end_angle)

        painter.save()
        painter.translate(cx, cy)
        painter.rotate(angle)
        painter.setPen(QPen(Qt.red, 4))
        painter.drawLine(0, 0, 0, -radius)
        painter.restore()