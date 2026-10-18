import QtQuick
import PzAnimator

// The Windows 7 progress bar: glossy green fill with the light sweep travelling across it.
Item {
    id: bar
    property real value: 0      // 0..1
    property bool active: true  // runs the sweep animation

    implicitHeight: 15
    implicitWidth: 160

    Rectangle {
        anchors.fill: parent
        radius: 3
        border.color: "#BCBCBC"
        gradient: Gradient {
            GradientStop { position: 0; color: "#F3F3F3" }
            GradientStop { position: 0.5; color: "#EAEAEA" }
            GradientStop { position: 1; color: "#DCDCDC" }
        }
    }

    Item {
        id: fill
        x: 1
        y: 1
        height: parent.height - 2
        width: Math.max(0, (parent.width - 2) * Math.min(1, Math.max(0, bar.value)))
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#C6F5C0" }
                GradientStop { position: 0.45; color: "#4FCB4B" }
                GradientStop { position: 0.5; color: "#22AE36" }
                GradientStop { position: 1.0; color: "#56D457" }
            }
        }

        // the sweeping highlight
        Rectangle {
            id: sweep
            width: 90
            height: parent.height
            visible: bar.active && fill.width > 4
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00FFFFFF" }
                GradientStop { position: 0.5; color: "#8CFFFFFF" }
                GradientStop { position: 1.0; color: "#00FFFFFF" }
            }
            NumberAnimation on x {
                running: sweep.visible
                loops: Animation.Infinite
                from: -sweep.width
                to: Math.max(bar.width, 200)
                duration: 2200
            }
        }
    }
}
