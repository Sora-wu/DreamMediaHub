import QtQuick

Item {
    id: root

    implicitWidth: 125
    implicitHeight: 26

    property real maxValue: 100
    property real currentVal: 0
    property bool showCurrentValue: false
    property real trackHeight: height
    property color backgroundColor: PlayerTheme.volumeBackgroundColor
    property color progressColor: PlayerTheme.volumeFillColor
    property string accessibleName: ""

    readonly property bool isDragging: mouseArea.pressed
    readonly property bool hovered: mouseArea.containsMouse
    readonly property real displayedValue: Math.max(0, Math.min(maxValue,
                                          isDragging ? dragValue : currentVal))
    readonly property real progress: maxValue > 0 ? displayedValue / maxValue : 0
    property real dragValue: 0

    signal sliderRelease(real value)

    Accessible.role: Accessible.Slider
    Accessible.name: accessibleName

    function valueAt(position) {
        return width > 0 && maxValue > 0
                ? Math.max(0, Math.min(1, position / width)) * maxValue : 0
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: root.trackHeight
        color: root.backgroundColor

        Rectangle {
            width: root.progress * parent.width
            height: parent.height
            color: root.progressColor
        }
    }

    Text {
        visible: root.showCurrentValue
        anchors.fill: parent
        text: Math.round(root.displayedValue).toString()
        color: PlayerTheme.textColor
        font: PlayerTheme.textFont
        renderType: Text.QtRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

        onPressed: function(mouse) {
            root.dragValue = root.valueAt(mouse.x)
        }
        onPositionChanged: function(mouse) {
            if (pressed)
                root.dragValue = root.valueAt(mouse.x)
        }
        onReleased: function(mouse) {
            root.dragValue = root.valueAt(mouse.x)
            root.sliderRelease(root.dragValue)
        }
        onWheel: function(wheel) {
            if (root.showCurrentValue && wheel.angleDelta.y !== 0) {
                root.sliderRelease(Math.max(0, Math.min(root.maxValue,
                                       root.currentVal + (wheel.angleDelta.y > 0 ? 5 : -5))))
                wheel.accepted = true
            } else {
                wheel.accepted = false
            }
        }
    }
}
