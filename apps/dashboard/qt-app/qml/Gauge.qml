import QtQuick

Item {
    id: root

    property string title: ""

    property real minimumValue: 0
    property real maximumValue: 100

    property real startAngle: -90
    property real endAngle: 90

    property real value: 0

    property string svgFile: ""

    width: 250
    height: 310

    readonly property real gaugeSize:
        Math.min(width, height - 60)

    readonly property real centerX:
        width / 2

    readonly property real centerY:
        10 + gaugeSize / 2

    readonly property real needleLength:
        gaugeSize / 2 - 10

    readonly property real fraction: {
        if (maximumValue === minimumValue)
            return 0

        return Math.max(
            0,
            Math.min(
                1,
                (value - minimumValue) /
                (maximumValue - minimumValue)
            )
        )
    }

    Image {
        id: gaugeBackground

        x: (root.width - root.gaugeSize) / 2
        y: 10

        width: root.gaugeSize
        height: root.gaugeSize

        source: root.svgFile

        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    Rectangle {
        id: needle

        width: 4
        height: root.needleLength

        color: "red"

        x: root.centerX - width / 2
        y: root.centerY - height

        transformOrigin: Item.Bottom

        rotation:
            root.startAngle +
            root.fraction *
            (root.endAngle - root.startAngle)

        antialiasing: true
    }

    Rectangle {
        id: needleCenter

        width: 12
        height: 12

        radius: 6

        color: "red"

        x: root.centerX - width / 2
        y: root.centerY - height / 2
    }

    Text {
        id: titleText

        anchors.centerIn: gaugeBackground

        text: root.title

        color: "gray"

        font.bold: true
        font.pointSize: 12
    }

    Text {
        id: valueText

        x: gaugeBackground.x
        y: gaugeBackground.y + gaugeBackground.height * 0.25

        width: gaugeBackground.width
        height: gaugeBackground.height

        text: Math.round(root.value)

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: "darkgreen"

        font.pointSize: 14
    }
}