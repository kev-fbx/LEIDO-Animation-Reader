import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers
import PzAnimator

ApplicationWindow {
    id: window

    required property AgentClient agent
    required property PoseState pose
    required property PzAssets assets
    required property AppSettings settings
    required property GameLauncher launcher
    required property TakeExporter exporter
    required property TakeLibrary library
    required property bool hotkeyReady

    width: 1360
    height: 820
    minimumWidth: 980
    minimumHeight: 600
    visible: true
    title: "LEIDO PZ Animation Exporter"
    color: Theme.window
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize

    readonly property bool inGame: agent.connected && agent.gameVersion !== ""

    function toggleRecording() {
        agent.toggleRecording(settings.rawTakesDir)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Aero glass caption band ------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 62
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.glassTop }
                GradientStop { position: 0.5; color: Theme.glassMid }
                GradientStop { position: 1.0; color: Theme.glassBottom }
            }
            Rectangle { width: parent.width; height: 1; color: "#FFFFFF"; opacity: 0.9 }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.glassEdge }
            // soft diagonal light streaks, like the glass frame reflections
            Rectangle {
                x: parent.width * 0.55; y: -40; width: 260; height: 160; rotation: 25; opacity: 0.35
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: "#00FFFFFF" }
                    GradientStop { position: 0.5; color: "#FFFFFF" }
                    GradientStop { position: 1; color: "#00FFFFFF" }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 16
                spacing: 12

                // glossy orb
                Rectangle {
                    implicitWidth: 40
                    implicitHeight: 40
                    radius: 20
                    border.color: "#1B4C86"
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#7DBEF3" }
                        GradientStop { position: 0.5; color: "#2C7BD1" }
                        GradientStop { position: 1; color: "#0F4A8E" }
                    }
                    Rectangle {
                        x: 6; y: 3; width: 28; height: 17; radius: 9
                        gradient: Gradient {
                            GradientStop { position: 0; color: "#E6FFFFFF" }
                            GradientStop { position: 1; color: "#10FFFFFF" }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "PZ"
                        font.family: Theme.fontFamily
                        font.pixelSize: 14
                        font.bold: true
                        color: "#FFFFFF"
                        style: Text.Raised
                        styleColor: "#0B3566"
                    }
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "LEIDO PZ Animation Exporter"
                        font.family: "Segoe UI Light"
                        font.pixelSize: 22
                        color: Theme.heading
                    }
                    Text {
                        text: "Record Project Zomboid animations for Blender"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSize
                        color: "#3D5A80"
                    }
                }

                Item { Layout.fillWidth: true }

                // connection status "pill"
                Rectangle {
                    implicitHeight: 28
                    implicitWidth: statusRow.implicitWidth + 24
                    radius: 14
                    color: "#66FFFFFF"
                    border.color: "#99FFFFFF"
                    RowLayout {
                        id: statusRow
                        anchors.centerIn: parent
                        spacing: 8
                        Rectangle {
                            implicitWidth: 12
                            implicitHeight: 12
                            radius: 6
                            border.color: window.inGame ? "#1C7A23" : window.agent.connected ? "#A57A12" : "#7A7A7A"
                            gradient: Gradient {
                                GradientStop { position: 0; color: window.inGame ? "#B8F5A8" : window.agent.connected ? "#FFE58A" : "#E2E2E2" }
                                GradientStop { position: 1; color: window.inGame ? "#2FA83A" : window.agent.connected ? "#E0A42A" : "#9A9A9A" }
                            }
                        }
                        Text {
                            text: window.inGame ? "Connected to Project Zomboid " + window.agent.gameVersion
                                : window.agent.connected ? "Game running - load a single-player save"
                                : "Not connected"
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSize
                            color: Theme.heading
                        }
                        Text {
                            visible: window.inGame && !window.agent.supportedBuild
                            text: "(unverified build)"
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSize
                            color: "#A0521C"
                        }
                    }
                }
            }
        }

        // ---- Explorer command bar ---------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 34
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.commandTop }
                GradientStop { position: 0.5; color: Theme.commandMid }
                GradientStop { position: 1.0; color: Theme.commandBottom }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.commandEdge }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 2

                AeroButton {
                    flat: true
                    iconKind: "play"
                    text: "Launch Project Zomboid"
                    enabled: !window.agent.connected && !window.launcher.busy
                    onClicked: window.launcher.launch()
                }
                CommandSeparator { }
                AeroButton {
                    flat: true
                    iconKind: window.agent.recording ? "stop" : "record"
                    text: window.agent.recording ? "Stop recording (F9)" : "Record (F9)"
                    enabled: window.inGame
                    onClicked: window.toggleRecording()
                }
                CommandSeparator { }
                AeroButton {
                    flat: true
                    iconKind: "folder"
                    text: "Open output folder"
                    onClicked: window.settings.openFolder(window.settings.outputDir)
                }
                AeroButton {
                    flat: true
                    text: "Change output folder..."
                    onClicked: folderDialog.open()
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: window.launcher.status !== "" && !window.agent.connected
                    text: window.launcher.status
                    elide: Text.ElideRight
                    Layout.maximumWidth: 520
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSize
                    color: "#A0521C"
                }
            }
        }

        // ---- Body: 3D view + takes pane --------------------------------------------------------
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 5
                color: SplitHandle.hovered || SplitHandle.pressed ? "#C5D9F1" : Theme.window
                Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 1; height: parent.height; color: "#C9D3DF" }
            }

            // 3D viewport over a Windows 7 "Harmony"-style wallpaper
            Item {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 420

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.wallpaperTop }
                        GradientStop { position: 0.55; color: Theme.wallpaperMid }
                        GradientStop { position: 1.0; color: Theme.wallpaperBottom }
                    }
                    // aurora light bands
                    Repeater {
                        model: [{ y: 0.30, r: -14, o: 0.22, h: 120 }, { y: 0.48, r: -9, o: 0.16, h: 70 }, { y: 0.62, r: -18, o: 0.10, h: 160 }]
                        Rectangle {
                            required property var modelData
                            x: -parent.width * 0.2
                            y: parent.height * modelData.y
                            width: parent.width * 1.4
                            height: modelData.h
                            rotation: modelData.r
                            opacity: modelData.o
                            gradient: Gradient {
                                GradientStop { position: 0; color: "#00BFE3FF" }
                                GradientStop { position: 0.5; color: "#BFE3FF" }
                                GradientStop { position: 1; color: "#00BFE3FF" }
                            }
                        }
                    }
                }

                View3D {
                    id: view
                    anchors.fill: parent
                    environment: SceneEnvironment {
                        backgroundMode: SceneEnvironment.Transparent
                        antialiasingMode: SceneEnvironment.MSAA
                        antialiasingQuality: SceneEnvironment.High
                    }

                    Node {
                        id: cameraPivot
                        position: window.pose.focus
                        PerspectiveCamera {
                            id: camera
                            position: Qt.vector3d(0, 120, 520)
                            eulerRotation.x: -12
                            clipNear: 1
                            clipFar: 50000
                        }
                    }

                    DirectionalLight {
                        eulerRotation.x: -45
                        eulerRotation.y: 35
                    }

                    // One grid cell = one PZ tile (100 scene units).
                    AxisHelper {
                        enableAxisLines: false
                        enableXZGrid: true
                        enableXYGrid: false
                        enableYZGrid: false
                        gridColor: "#8CC3F0"
                        gridOpacity: 0.35
                    }

                    Model {
                        visible: window.pose.hasPose
                        geometry: BoneGeometry { pose: window.pose }
                        materials: PrincipledMaterial {
                            baseColor: "#FFF3C4"
                            lighting: PrincipledMaterial.NoLighting
                        }
                    }
                }

                OrbitCameraController {
                    anchors.fill: view
                    origin: cameraPivot
                    camera: camera
                }

                // Aero glass info panel
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 12
                    radius: 6
                    implicitWidth: infoColumn.implicitWidth + 20
                    implicitHeight: infoColumn.implicitHeight + 14
                    color: "#33FFFFFF"
                    border.color: "#80FFFFFF"
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: 5
                        color: "transparent"
                        border.color: "#26000000"
                    }
                    ColumnLayout {
                        id: infoColumn
                        anchors.centerIn: parent
                        spacing: 2
                        GlassText {
                            text: window.inGame ? "Live view - " + window.agent.fps.toFixed(0) + " fps" : "Live view"
                            font.bold: true
                        }
                        GlassText {
                            visible: window.inGame
                            text: "Position  " + window.agent.position.x.toFixed(2) + ", "
                                  + window.agent.position.y.toFixed(2) + "   level " + window.agent.position.z.toFixed(0)
                        }
                        GlassText {
                            visible: !window.pose.hasPose
                            text: window.agent.connected ? "Waiting for the character..." : "Launch the game to see your character"
                        }
                        GlassText { text: window.assets.report; font.pixelSize: 11; opacity: 0.85 }
                    }
                }

                // recording badge
                Rectangle {
                    visible: window.agent.recording
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 12
                    radius: 4
                    implicitWidth: recRow.implicitWidth + 18
                    implicitHeight: 26
                    color: "#CC8E0B0B"
                    border.color: "#FFB0A0"
                    RowLayout {
                        id: recRow
                        anchors.centerIn: parent
                        spacing: 6
                        AeroIcon {
                            kind: "record"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: window.agent.recording
                                NumberAnimation { to: 0.25; duration: 600 }
                                NumberAnimation { to: 1.0; duration: 600 }
                            }
                        }
                        GlassText { text: "REC"; font.bold: true }
                    }
                }
            }

            TakesPane {
                SplitView.preferredWidth: 460
                SplitView.minimumWidth: 380
                library: window.library
                exporter: window.exporter
                settings: window.settings
            }
        }

        // ---- Status bar ----------------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 26
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.statusTop }
                GradientStop { position: 1; color: Theme.statusBottom }
            }
            Rectangle { width: parent.width; height: 1; color: Theme.statusEdge }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 12
                StatusText {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: window.agent.recording ? "Recording... press F9 again to stop"
                        : window.exporter.status !== "" ? window.exporter.status
                        : window.hotkeyReady ? "Ready. F9 starts and stops recording, even while the game has focus."
                        : "Ready. F9 is taken by another program; use the Record button."
                }
                StatusSeparator { }
                StatusText {
                    text: "Output: " + window.settings.outputDir
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: 420
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Choose the output folder for takes and GLB files"
        currentFolder: "file:///" + window.settings.outputDir
        onAccepted: window.settings.setOutputDirUrl(selectedFolder)
    }

    component CommandSeparator: Rectangle {
        implicitWidth: 1
        implicitHeight: 20
        color: "#B9C9DC"
        Layout.leftMargin: 4
        Layout.rightMargin: 4
    }
    component StatusSeparator: Rectangle {
        implicitWidth: 1
        implicitHeight: 16
        color: "#B7C4D5"
    }
    component StatusText: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSize
        color: "#1E2F44"
    }
    component GlassText: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSize
        color: "#FFFFFF"
        style: Text.Raised
        styleColor: "#55002244"
    }
}
