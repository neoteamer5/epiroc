from PySide6.QtWidgets import QLabel
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtGui import QPainter
from PySide6.QtCore import QSize
from PySide6.QtCore import Qt
from PySide6.QtCore import QRect


class WarningWidget(QLabel):
    def __init__(self, svg_path="gauges/warning.svg"):
        super().__init__()
        self.renderer = QSvgRenderer(svg_path)
        self.setFixedSize(QSize(110, 110))   # small icon
        self.active = False

    def set_state(self, active):
        self.active = active
        self.update()   # triggers paintEvent

    def paintEvent(self, event):
        painter = QPainter(self)

        # Draw SVG centered in the top 60% of the widget
        icon_rect = QRect(0, 0, self.width(), int(self.height() * 0.6))
        
        # Draw icon only when active
        if self.active:
            self.renderer.render(painter, icon_rect)

        # Draw text in the bottom 40%
        text_rect = QRect(0, int(self.height() * 0.6), self.width(), int(self.height() * 0.4))

        if self.active:
            painter.drawText(text_rect, Qt.AlignCenter, "WARNING ACTIVE!")
        else:
            painter.drawText(text_rect, Qt.AlignCenter, "OK. No warning.")
