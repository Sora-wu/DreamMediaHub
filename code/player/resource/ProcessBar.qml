import QtQuick

Rectangle {
    id: root
    width: 1280
    height: 50

    property real progress: 0.3
    property real maxValue: 300         // 做为音量条表示最大音量，作为视频条表示视频时长
    property real currentVal: 0
    property bool showCurrentValue: false

    Rectangle {
        id: background
        color: "#2f343f"
        anchors.fill: parent

        Rectangle {
            id: frontground
            color: "#46595c"
            anchors.left: parent.left
            anchors.leftMargin: 0
            height: parent.height
            width: root.progress * parent.width
        }

        MouseArea {
            anchors.fill: parent
            onPositionChanged: function(mouse) {
                var progress = mouse.x / parent.width
                root.progress = Math.max(0, Math.min(1, progress))
                root.currentVal = root.progress * maxValue;
            }
            onReleased: function(mouse) {
                var currentProgress = mouse.x / parent.width
                root.progress = Math.max(0, Math.min(1, currentProgress))
                root.currentVal = root.progress * maxValue;
            }
        }

        Text {
            id: txtNum
            color: "#ffffff"
            text: currentVal.toFixed()
            visible: showCurrentValue
            anchors.fill: parent
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            fontSizeMode: Text.HorizontalFit
            style: Text.Outline
        }
    }
}
