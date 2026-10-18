import QtQuick
import QtQuick.Controls.Basic
import PzAnimator

// Windows 7 group box: pale blue-grey rounded border with the caption cut into it.
GroupBox {
    id: control
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    topPadding: 22
    leftPadding: 10
    rightPadding: 10
    bottomPadding: 10

    label: Text {
        x: 8
        text: control.title
        font: control.font
        color: Theme.heading
        leftPadding: 3
        rightPadding: 3
        Rectangle {
            z: -1
            anchors.fill: parent
            color: Theme.window
        }
    }

    background: Rectangle {
        y: 8
        width: control.width
        height: control.height - 8
        radius: 3
        color: "transparent"
        border.color: Theme.groupBorder
    }
}
