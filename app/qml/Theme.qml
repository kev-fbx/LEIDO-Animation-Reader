pragma Singleton
import QtQuick

// Windows 7 (Aero) palette and metrics, taken from the classic Explorer / dialog look.
QtObject {
    readonly property string fontFamily: "Segoe UI"
    readonly property int fontSize: 12            // 9 pt at 96 dpi
    readonly property color text: "#000000"
    readonly property color textDisabled: "#838383"
    readonly property color textMuted: "#5B6B80"
    readonly property color heading: "#1E395B"     // the Explorer / Control Panel headline blue
    readonly property color link: "#0066CC"

    readonly property color window: "#F0F0F0"
    readonly property color content: "#FFFFFF"

    // Aero glass caption band
    readonly property color glassTop: "#E4EFFB"
    readonly property color glassMid: "#CFE0F5"
    readonly property color glassBottom: "#BAD1EE"
    readonly property color glassEdge: "#8FA7C6"

    // Explorer command bar
    readonly property color commandTop: "#FDFEFF"
    readonly property color commandMid: "#EEF4FB"
    readonly property color commandBottom: "#DCE6F4"
    readonly property color commandEdge: "#A0AFC3"

    // Push buttons: border + four gradient stops (top, mid-top, mid-bottom, bottom)
    readonly property var buttonNormal:  ["#707070", "#F2F2F2", "#EBEBEB", "#DDDDDD", "#CFCFCF"]
    readonly property var buttonHover:   ["#3C7FB1", "#EAF6FD", "#D9F0FC", "#BEE6FD", "#A7D9F5"]
    readonly property var buttonPressed: ["#2C628B", "#E5F4FC", "#C4E5F6", "#98D1EF", "#68B3DB"]
    readonly property var buttonDisabled:["#ADB2B5", "#F4F4F4", "#F4F4F4", "#F4F4F4", "#F4F4F4"]

    // Command-bar (flat) buttons
    readonly property var flatHover:   ["#7DA2CE", "#F2F8FE", "#DCEBFC", "#C9E0FC", "#C1DBFC"]
    readonly property var flatPressed: ["#5E87B8", "#D5E6FB", "#C2DCFD", "#B1D1FB", "#A9CCFA"]

    // Explorer list selection
    readonly property color itemHoverBorder: "#B8D6FB"
    readonly property color itemHoverTop: "#FAFBFD"
    readonly property color itemHoverBottom: "#EBF3FD"
    readonly property color itemSelectedBorder: "#84ACDD"
    readonly property color itemSelectedTop: "#F1F7FE"
    readonly property color itemSelectedBottom: "#D5E7FC"

    readonly property color groupBorder: "#D5DFE5"
    readonly property color fieldBorder: "#ABADB3"
    readonly property color fieldFocus: "#3D7BAD"
    readonly property color separator: "#D5D5D5"

    // Status bar / details pane
    readonly property color statusTop: "#F7FAFD"
    readonly property color statusBottom: "#E4EBF4"
    readonly property color statusEdge: "#C5D2E2"

    // Harmony-style desktop backdrop behind the 3D view
    readonly property color wallpaperTop: "#0E3B73"
    readonly property color wallpaperMid: "#2E78C4"
    readonly property color wallpaperBottom: "#0B2E5C"
}
