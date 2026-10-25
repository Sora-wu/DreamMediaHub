pragma Singleton

import QtQuick

QtObject {
    id: theme

    readonly property color panelColor: "#2e3440"
    readonly property color statusColor: "#2f343f"
    readonly property color textColor: "#d8dee9"
    readonly property color separatorColor: "#4c566a"
    readonly property color hoverColor: "#3b4252"
    readonly property color pressedColor: "#434c5e"
    readonly property color seekFillColor: "#4c566a"
    readonly property color seekBackgroundColor: panelColor
    readonly property color volumeBackgroundColor: "#2f343f"
    readonly property color volumeFillColor: "#4c566a"

    // This face includes Latin and CJK glyphs, without a system font fallback.
    readonly property FontLoader uiFont: FontLoader {
        source: Qt.resolvedUrl("fonts/NotoSansCJKsc-Regular.otf")
    }
    readonly property font textFont: Qt.font({
        family: theme.uiFont.name,
        pixelSize: 14,
        weight: Font.Normal,
        hintingPreference: Font.PreferNoHinting
    })
}
