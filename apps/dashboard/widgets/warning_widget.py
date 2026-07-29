from PySide6.QtWidgets import QLabel
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtGui import QPainter
from PySide6.QtCore import QSize
from PySide6.QtCore import Qt

class WarningWidget(QLabel):
    def __init__(self, svg_path="gauges/warning.svg"):
        super().__init__()
        self.renderer = QSvgRenderer(svg_path)
        self.setFixedSize(QSize(80, 80))   # small icon
        self.active = False

    def set_state(self, active):
        self.active = active
        self.update()   # triggers paintEvent

    def paintEvent(self, event):
            painter = QPainter(self)

            # Draw SVG scaled to widget size
            self.renderer.render(painter)

            # Draw overlay text
            if self.active:
                painter.drawText(self.rect(), Qt.AlignCenter, "ACTIVE")
            else:
                painter.drawText(self.rect(), Qt.AlignCenter, "OK")
