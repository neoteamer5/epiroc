import QtQuick

Item {
    id: root

    property bool active: false
    property string svgFile: "../gauges/warning.svg"

    width: 110
    height: 110

    Image {
        id: warningIcon

        x: 0
        y: 0

        width: root.width
        height: root.height * 0.6

        source: root.svgFile

        fillMode: Image.PreserveAspectFit
        visible: root.active
        smooth: true
    }

    Text {
        id: warningText

        x: 0
        y: root.height * 0.6

        width: root.width
        height: root.height * 0.4

        text: root.active
              ? "Temp > 120"
              : "Temp OK."

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        font.pointSize: 11
    }
}