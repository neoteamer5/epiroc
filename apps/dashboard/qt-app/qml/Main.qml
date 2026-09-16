import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    visible: true
    width: 900
    height: 600

    title: "Vehicle Dashboard"

    Connections {
        target: dashboardBackend

        function onValuesChanged(spd, rpm, fuel, temp, warn) {
            speedGauge.value = spd
            rpmGauge.value = rpm
            fuelGauge.value = fuel
            coolantGauge.value = temp
            warning.active = warn
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10

        columns: 2

        rowSpacing: 10
        columnSpacing: 10

        Gauge {
            id: speedGauge

            Layout.fillWidth: true
            Layout.fillHeight: true

            title: "Speed"
            minimumValue: 0
            maximumValue: 180
            startAngle: 0
            endAngle: 325
            svgFile: "../gauges/speedometer.svg"
        }

        Gauge {
            id: rpmGauge

            Layout.fillWidth: true
            Layout.fillHeight: true

            title: "RPM"
            minimumValue: 0
            maximumValue: 5000
            startAngle: 0
            endAngle: 180
            svgFile: "../gauges/tachometer.svg"
        }

        Gauge {
            id: coolantGauge

            Layout.fillWidth: true
            Layout.fillHeight: true

            title: "Coolant"
            minimumValue: 0
            maximumValue: 150
            startAngle: -90
            endAngle: 90
            svgFile: "../gauges/coolant.svg"
        }

        Gauge {
            id: fuelGauge

            Layout.fillWidth: true
            Layout.fillHeight: true

            title: "Fuel"
            minimumValue: 0
            maximumValue: 100
            startAngle: -90
            endAngle: 90
            svgFile: "../gauges/fuel.svg"
        }

        Warning {
            id: warning

            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter

            svgFile: "../gauges/warning.svg"
        }
    }
}