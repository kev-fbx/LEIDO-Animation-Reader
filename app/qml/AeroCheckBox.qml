import QtQuick
import QtQuick.Controls.Basic
import PzAnimator

// Windows 7 check box: 13 px square, soft gradient, blue border on hover, dark-blue tick.
CheckBox {
    id: control
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    hoverEnabled: true
    spacing: 6
    padding: 2

    indicator: Rectangle {
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: 13
        height: 13
        border.color: control.hovered ? "#3C7FB1" : "#8E8F8F"
        gradient: Gradient {
            GradientStop { position: 0; color: control.down ? "#B7DEF6" : control.hovered ? "#DEF2FC" : "#F2F2F2" }
            GradientStop { position: 1; color: control.down ? "#E3F3FC" : control.hovered ? "#F5FBFE" : "#FFFFFF" }
        }
        Canvas {
            anchors.fill: parent
            visible: control.checked
            onVisibleChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.beginPath()
                ctx.moveTo(2.5, 6.5)
                ctx.lineTo(5.5, 9.5)
                ctx.lineTo(10.5, 3)
                ctx.lineWidth = 2
                ctx.strokeStyle = "#3E5D8E"
                ctx.stroke()
            }
        }
    }

    contentItem: Text {
        leftPadding: control.text === "" ? 0 : control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
