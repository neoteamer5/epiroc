from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout
from PySide6.QtGui import QPainter, QPen
from PySide6.QtCore import Qt, QRectF
import math

class GaugeWidget(QWidget):
    def __init__(self, title, min_val, max_val, start_angle, end_angle):
        super().__init__()
        self.title = title
        self.min_val = min_val
        self.max_val = max_val
        self.start_angle = start_angle      # e.g., -130 degrees
        self.end_angle = end_angle          # e.g., +130 degrees
        self.value = 0

        # Numeric value label
        #self.value_label = QLabel("0")
        #self.value_label.setAlignment(Qt.AlignCenter)

        # Layout: title → gauge → value
        layout = QVBoxLayout(self)
        layout.addStretch()
        #layout.addWidget(self.value_label)

    def set_value(self, val):
        self.value = val
        #self.value_label.setText(str(val))
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Perfect circle
        size = min(self.width(), self.height() - 60)
        rect = QRectF(
            (self.width() - size) / 2,
            10,
            size,
            size
        )

        # Draw gauge arc
        painter.setPen(QPen(Qt.gray, 8))
        painter.drawArc(rect, self.start_angle * 16, (self.end_angle - self.start_angle) * 16)

        # Compute needle angle
        angle_range = self.end_angle - self.start_angle
        frac = (self.value - self.min_val) / (self.max_val - self.min_val)
        needle_angle = math.radians(self.start_angle + frac * angle_range)

        # Needle center
        cx = rect.center().x()
        cy = rect.center().y()

        needle_length = size / 2 - 10

        x2 = cx + needle_length * math.cos(needle_angle)
        y2 = cy + needle_length * math.sin(needle_angle)

        # Draw needle
        painter.setPen(QPen(Qt.red, 4))
        painter.drawLine(int(cx), int(cy), int(x2), int(y2))

        # Draw gauge title in center of circle (bold + gray)
        painter.setPen(QPen(Qt.gray, 2))

        font = painter.font()
        font.setBold(True)
        font.setPointSize(12)
        painter.setFont(font)

        painter.drawText(rect, Qt.AlignCenter, self.title)

        # Draw numeric value BELOW the title, but ABOVE the bottom of the circle
        value_rect = QRectF(
            rect.x(),
            rect.y() + size * 0.25,   # 55% down from top of circle
            rect.width(),
            rect.height()
        )

        # GREEN value text
        painter.setPen(QPen(Qt.darkGreen, 2))
        font.setBold(False)
        font.setPointSize(14)
        painter.setFont(font)
        painter.drawText(value_rect, Qt.AlignCenter, str(self.value))

