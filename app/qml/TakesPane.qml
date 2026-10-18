import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import PzAnimator

// Explorer-style list of recorded takes with GLB export options.
Rectangle {
    id: pane

    required property TakeLibrary library
    required property TakeExporter exporter
    required property AppSettings settings

    color: Theme.window

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Heading, Control Panel style
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Recorded takes"
                font.family: Theme.fontFamily
                font.pixelSize: 17
                color: Theme.heading
            }
            Item { Layout.fillWidth: true }
            Text {
                text: pane.library.count + (pane.library.count === 1 ? " item" : " items")
                      + (pane.library.checkedCount > 0 ? "   " + pane.library.checkedCount + " selected" : "")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSize
                color: Theme.textMuted
            }
        }

        // The list, framed like an Explorer content view
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.content
            border.color: "#828790"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                // Column headers
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 24
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#FFFFFF" }
                        GradientStop { position: 1; color: "#F1F3F6" }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.separator }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        spacing: 0
                        AeroCheckBox {
                            Layout.preferredWidth: 26
                            checked: pane.library.count > 0 && pane.library.checkedCount === pane.library.count
                            onClicked: pane.library.setAllChecked(checked)
                        }
                        HeaderCell { text: "Name"; Layout.fillWidth: true }
                        HeaderCell { text: "Length"; Layout.preferredWidth: 58 }
                        HeaderCell { text: "Size"; Layout.preferredWidth: 70 }
                        HeaderCell { text: "GLB"; Layout.preferredWidth: 40 }
                    }
                }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: pane.library
                    currentIndex: -1
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Item {
                        id: row
                        required property int index
                        required property string name
                        required property string path
                        required property string modified
                        required property string sizeText
                        required property string duration
                        required property bool hasGlb
                        required property bool checked

                        width: ListView.view.width
                        height: 40
                        readonly property bool selected: ListView.isCurrentItem

                        // Explorer hover / selection highlight
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            radius: 3
                            visible: hover.hovered || row.selected
                            border.color: row.selected ? Theme.itemSelectedBorder : Theme.itemHoverBorder
                            gradient: Gradient {
                                GradientStop { position: 0; color: row.selected ? Theme.itemSelectedTop : Theme.itemHoverTop }
                                GradientStop { position: 1; color: row.selected ? Theme.itemSelectedBottom : Theme.itemHoverBottom }
                            }
                        }
                        HoverHandler { id: hover }
                        TapHandler {
                            onTapped: list.currentIndex = row.index
                            onDoubleTapped: pane.exporter.exportTakes([row.path])
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 4
                            spacing: 0
                            AeroCheckBox {
                                Layout.preferredWidth: 26
                                checked: row.checked
                                onClicked: pane.library.setChecked(row.index, checked)
                            }
                            AeroIcon { kind: "take"; Layout.rightMargin: 6 }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    Layout.fillWidth: true
                                    text: row.name
                                    elide: Text.ElideRight
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSize
                                    color: Theme.text
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: row.modified
                                    elide: Text.ElideRight
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                }
                            }
                            CellText { text: row.duration; Layout.preferredWidth: 58 }
                            CellText { text: row.sizeText; Layout.preferredWidth: 70 }
                            Item {
                                Layout.preferredWidth: 40
                                Layout.fillHeight: true
                                AeroIcon { kind: "check"; visible: row.hasGlb; anchors.centerIn: parent }
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        width: parent.width - 40
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "No takes yet.\nPress F9 in game to record one."
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSize
                        color: Theme.textMuted
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AeroButton {
                text: "Export selected (" + pane.library.checkedCount + ")"
                iconKind: "export"
                highlighted: true
                enabled: pane.library.checkedCount > 0
                onClicked: pane.exporter.exportTakes(pane.library.checkedPaths())
            }
            AeroButton {
                text: "Export new"
                enabled: pane.library.count > 0
                onClicked: pane.exporter.exportTakes(pane.library.pathsWithoutGlb())
            }
            AeroButton {
                text: "Other file..."
                iconKind: "file"
                onClicked: fileDialog.open()
            }
            Item { Layout.fillWidth: true }
        }

        AeroGroupBox {
            Layout.fillWidth: true
            title: "Export options"

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 12
                rowSpacing: 4

                OptionLabel { text: "Frame rate:" }
                RowLayout {
                    spacing: 10
                    Repeater {
                        model: [24, 30, 60]
                        AeroRadioButton {
                            required property int modelData
                            text: modelData + " fps"
                            checked: pane.settings.exportFps === modelData
                            onClicked: pane.settings.exportFps = modelData
                        }
                    }
                }

                OptionLabel { text: "Root motion:" }
                RowLayout {
                    spacing: 10
                    AeroRadioButton {
                        text: "Moves through the world"
                        checked: pane.settings.exportWorldRoot
                        onClicked: pane.settings.exportWorldRoot = true
                    }
                    AeroRadioButton {
                        text: "In place"
                        checked: !pane.settings.exportWorldRoot
                        onClicked: pane.settings.exportWorldRoot = false
                    }
                }

                OptionLabel { text: "Scale:" }
                RowLayout {
                    spacing: 6
                    AeroTextField {
                        Layout.preferredWidth: 56
                        text: pane.settings.exportScale.toFixed(2)
                        validator: DoubleValidator { bottom: 0.1; top: 10; decimals: 3 }
                        onEditingFinished: pane.settings.exportScale = Number(text)
                    }
                    OptionLabel {
                        text: "metres per model unit (height " + (0.98 * pane.settings.exportScale).toFixed(2) + " m)"
                        color: Theme.textMuted
                    }
                }

                Item { implicitWidth: 1 }
                AeroCheckBox {
                    text: "Export automatically when recording stops"
                    checked: pane.settings.autoExport
                    onClicked: pane.settings.autoExport = checked
                }
            }
        }

        // Progress, Windows 7 copy-dialog style
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            visible: pane.exporter.busy || pane.exporter.status !== ""
            Text {
                Layout.fillWidth: true
                text: pane.exporter.status
                elide: Text.ElideMiddle
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            AeroProgressBar {
                Layout.fillWidth: true
                visible: pane.exporter.busy
                value: pane.exporter.total > 0 ? (pane.exporter.done + 0.5) / pane.exporter.total : 0
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Export takes to GLB"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["LEIDO PZ Animation Exporter takes (*.pzrec)", "All files (*)"]
        currentFolder: "file:///" + pane.settings.rawTakesDir
        onAccepted: pane.exporter.exportUrls(selectedFiles)
    }

    component HeaderCell: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSize
        color: "#4C607A"
        leftPadding: 6
        verticalAlignment: Text.AlignVCenter
        Layout.fillHeight: true
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height - 6; anchors.verticalCenter: parent.verticalCenter; color: "#E3E7EC" }
    }
    component CellText: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSize
        color: Theme.textMuted
        leftPadding: 6
    }
    component OptionLabel: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
}
