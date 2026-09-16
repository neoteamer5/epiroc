import sys

from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine

from can_reader import CANReader


class DashboardBackend(QObject):
    valuesChanged = Signal(int, int, int, int, bool)

    def __init__(self, use_demo):
        super().__init__()

        self.reader = CANReader(
            self.valuesChanged,
            use_demo
        )


if __name__ == "__main__":
    use_demo = False

    if len(sys.argv) > 1:
        arg = sys.argv[1].strip().lower()

        if arg == "demo":
            use_demo = True
        elif arg == "plc":
            use_demo = False
        else:
            print(
                f"Unknown mode '{arg}', "
                "expected 'demo' or 'PLC'."
            )

    app = QGuiApplication(sys.argv)

    backend = DashboardBackend(use_demo)

    engine = QQmlApplicationEngine()

    engine.rootContext().setContextProperty(
        "dashboardBackend",
        backend
    )

    engine.load("qml/Main.qml")

    if not engine.rootObjects():
        sys.exit(-1)

    sys.exit(app.exec())