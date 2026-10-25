import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Button {
    id: root

    property string symbol: "play"
    property int symbolSize: 20
    property string description: ""

    implicitWidth: 44
    implicitHeight: 36
    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: implicitHeight
    Layout.alignment: Qt.AlignVCenter
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.name: description

    background: Rectangle {
        radius: 2
        color: root.down ? PlayerTheme.pressedColor
                        : root.hovered ? PlayerTheme.hoverColor : "transparent"
        border.width: root.visualFocus ? 1 : 0
        border.color: PlayerTheme.textColor
    }

    contentItem: Item {
        PlayerIcon {
            anchors.centerIn: parent
            width: root.symbolSize
            height: root.symbolSize
            symbol: root.symbol
            color: PlayerTheme.textColor
        }
    }

    ToolTip {
        visible: root.hovered && root.description.length > 0
        delay: 600
        text: root.description
        padding: 6
        background: Rectangle {
            color: PlayerTheme.panelColor
            border.color: PlayerTheme.separatorColor
            radius: 2
        }
        contentItem: Text {
            text: root.description
            font: PlayerTheme.textFont
            renderType: Text.QtRendering
            color: PlayerTheme.textColor
        }
    }
}
