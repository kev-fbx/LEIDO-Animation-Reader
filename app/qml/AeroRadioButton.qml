import QtQuick
import QtQuick.Controls.Basic
import PzAnimator

// Windows 7 radio button: round, light gradient, glossy blue dot when selected.
RadioButton {
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
        radius: 6.5
        border.color: control.hovered ? "#3C7FB1" : "#8E8F8F"
        gradient: Gradient {
            GradientStop { position: 0; color: control.hovered ? "#DEF2FC" : "#EDEDED" }
            GradientStop { position: 1; color: "#FFFFFF" }
        }
        Rectangle {
            visible: control.checked
            anchors.centerIn: parent
            width: 7
            height: 7
            radius: 3.5
            border.color: "#1E4E7E"
            gradient: Gradient {
                GradientStop { position: 0; color: "#A6D8F8" }
                GradientStop { position: 1; color: "#2B70B5" }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
    }
}
