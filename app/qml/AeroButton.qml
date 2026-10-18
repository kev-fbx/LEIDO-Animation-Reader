import QtQuick
import QtQuick.Controls.Basic
import PzAnimator

// Windows 7 push button: grey glossy gradient, blue glow on hover, darker blue when pressed.
// flat (Button's own property): Explorer command-bar style, frameless until hovered.
Button {
    id: control

    property string iconKind: ""

    readonly property var scheme: !enabled ? Theme.buttonDisabled
                                 : down ? (flat ? Theme.flatPressed : Theme.buttonPressed)
                                 : hovered ? (flat ? Theme.flatHover : Theme.buttonHover)
                                 : Theme.buttonNormal
    readonly property bool framed: !flat || hovered || down

    implicitHeight: flat ? 26 : 24
    implicitWidth: Math.max(flat ? 0 : 75, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: flat ? 8 : 12
    rightPadding: flat ? 8 : 12
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    hoverEnabled: true

    contentItem: Row {
        spacing: 6
        AeroIcon {
            kind: control.iconKind
            visible: kind !== ""
            dimmed: !control.enabled
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: control.text
            font: control.font
            color: control.enabled ? Theme.text : Theme.textDisabled
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: 3
            visible: control.framed
            border.color: control.scheme[0]
            gradient: Gradient {
                GradientStop { position: 0.0; color: control.scheme[1] }
                GradientStop { position: 0.48; color: control.scheme[2] }
                GradientStop { position: 0.5; color: control.scheme[3] }
                GradientStop { position: 1.0; color: control.scheme[4] }
            }
        }
        // the 1 px white inner rim that makes Aero buttons look raised
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 2
            color: "transparent"
            border.color: "#B4FFFFFF"
            visible: control.framed && control.enabled && !control.down
        }
        // default-button glow
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 2
            color: "transparent"
            border.color: "#6ACBF5"
            visible: control.highlighted && !control.flat && control.enabled && !control.down && !control.hovered
        }
    }
}
