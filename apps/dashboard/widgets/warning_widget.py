from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout
from PySide6.QtSvgWidgets import QSvgWidget

class WarningWidget(QWidget):
    def __init__(self, svg_path):
        super().__init__()
        layout = QVBoxLayout()
        self.svg = QSvgWidget(svg_path)
        self.label = QLabel("Warnings: None")
        layout.addWidget(self.svg)
        layout.addWidget(self.label)
        self.setLayout(layout)

    def set_state(self, active):
        self.label.setText("Warnings: ACTIVE" if active else "Warnings: None")
