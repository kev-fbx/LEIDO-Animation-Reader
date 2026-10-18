import QtQuick
import QtQuick.Controls.Basic
import PzAnimator

// Windows 7 text box: grey border, blue when hovered or focused.
TextField {
    id: control
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    color: Theme.text
    selectionColor: "#3399FF"
    selectedTextColor: "#FFFFFF"
    implicitHeight: 23
    leftPadding: 4
    rightPadding: 4
    topPadding: 2
    bottomPadding: 2
    hoverEnabled: true

    background: Rectangle {
        radius: 2
        color: control.enabled ? Theme.content : "#F4F4F4"
        border.color: control.activeFocus ? Theme.fieldFocus : control.hovered ? "#5794BF" : Theme.fieldBorder
    }
}
