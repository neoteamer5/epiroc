from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout
from PySide6.QtSvgWidgets import QSvgWidget

class GaugeWidget(QWidget):
    def __init__(self, svg_path, label, min_val, max_val):
        super().__init__()
        self.min = min_val
        self.max = max_val

        layout = QVBoxLayout()
        self.svg = QSvgWidget(svg_path)
        self.text = QLabel(f"{label}: 0")
        layout.addWidget(self.svg)
        layout.addWidget(self.text)
        self.setLayout(layout)

    def set_value(self, val):
        self.text.setText(f"{val}")
