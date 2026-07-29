from PySide6.QtWidgets import QWidget
from PySide6.QtGui import QPainter, QPen
from PySide6.QtCore import Qt

def value_to_angle(val, vmin, vmax, amin, amax):
    if val < vmin: val = vmin
    if val > vmax: val = vmax
    ratio = (val - vmin) / (vmax - vmin)
    return amin + ratio * (amax - amin)

class GaugeWidget(QWidget):
    def __init__(self, label, vmin, vmax, amin, amax):
        super().__init__()
        self.label = label
        self.vmin = vmin
        self.vmax = vmax
        self.amin = amin
        self.amax = amax
        self.value = vmin

    def set_value(self, val):
        self.value = val
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        w = self.width()
        h = self.height()
        cx = w / 2
        cy = h / 2
        radius = min(w, h) * 0.4

        # dial circle
        painter.setPen(QPen(Qt.white, 3))
        painter.drawEllipse(cx - radius, cy - radius, 2 * radius, 2 * radius)

        # needle angle
        angle = value_to_angle(self.value, self.vmin, self.vmax, self.amin, self.amax)

        painter.save()
        painter.translate(cx, cy)
        painter.rotate(angle)
        painter.setPen(QPen(Qt.red, 4))
        painter.drawLine(0, 0, 0, -radius)
        painter.restore()

