import QtQuick
import QtQuick.Layouts
import Dream.play

Window {
    id: root

    width: 1280
    height: 720
    minimumWidth: 640
    minimumHeight: 360
    visible: true
    color: "#000000"
    title: playerCtrl.mediaName.length > 0
           ? playerCtrl.mediaName + " — DreamPlayer" : qsTr("DreamPlayer")

    property int volumeBeforeMute: 100

    PlayerController {
        id: playerCtrl
    }

    Shortcut {
        sequence: "Left"
        onActivated: playerCtrl.seek(-5)
    }

    Shortcut {
        sequence: "Right"
        onActivated: playerCtrl.seek(5)
    }

    Shortcut {
        sequence: "Space"
        onActivated: playerCtrl.togglePlay()
    }

    Shortcut {
        sequence: "["
        onActivated: playerCtrl.speed = Math.max(playerCtrl.speed - 0.1, 0.5)
    }

    Shortcut {
        sequence: "]"
        onActivated: playerCtrl.speed = Math.min(playerCtrl.speed + 0.1, 2.0)
    }

    VideoArea {
        id: videoArea
        objectName: "videoArea"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomBar.top
        controller: playerCtrl
    }

    Rectangle {
        id: bottomBar
        objectName: "bottomBar"
        height: 76
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: PlayerTheme.panelColor

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 28
            color: PlayerTheme.statusColor
        }

        RowLayout {
            id: controlRow
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 4
            height: 38
            spacing: 4

            PlaybackButton {
                objectName: "playPauseButton"
                symbol: playerCtrl.isPlaying ? "pause" : "play"
                description: playerCtrl.isPlaying ? qsTr("暂停（空格）") : qsTr("播放（空格）")
                onClicked: playerCtrl.togglePlay()
            }

            // Playlist navigation remains a placeholder, as in the original UI.
            PlaybackButton {
                objectName: "previousButton"
                symbol: "previous"
                description: qsTr("上一个")
            }

            PlaybackButton {
                objectName: "nextButton"
                symbol: "next"
                description: qsTr("下一个")
            }

            ProcessBar {
                id: videoBar
                objectName: "videoBar"
                Layout.fillWidth: true
                Layout.minimumWidth: 80
                Layout.preferredHeight: 26
                Layout.rightMargin: 8
                Layout.alignment: Qt.AlignVCenter
                enabled: maxValue > 0
                currentVal: playerCtrl.currentProgress
                maxValue: playerCtrl.duration
                backgroundColor: PlayerTheme.seekBackgroundColor
                progressColor: PlayerTheme.seekFillColor
                accessibleName: qsTr("播放进度")

                onSliderRelease: function(value) {
                    playerCtrl.seek(Math.round(value), false)
                }
            }

            PlaybackButton {
                objectName: "muteButton"
                Layout.preferredWidth: 36
                symbolSize: 24
                symbol: playerCtrl.volume === 0 ? "mute" : "volume"
                description: playerCtrl.volume === 0 ? qsTr("取消静音") : qsTr("静音")
                onClicked: {
                    if (playerCtrl.volume > 0) {
                        root.volumeBeforeMute = playerCtrl.volume
                        playerCtrl.setVolume(0)
                    } else {
                        playerCtrl.setVolume(root.volumeBeforeMute)
                    }
                }
            }

            ProcessBar {
                id: volumeBar
                objectName: "volumeBar"
                Layout.preferredWidth: 125
                Layout.preferredHeight: 26
                Layout.alignment: Qt.AlignVCenter
                currentVal: playerCtrl.volume
                maxValue: 100
                trackHeight: height
                showCurrentValue: true
                accessibleName: qsTr("音量")

                onSliderRelease: function(value) {
                    playerCtrl.setVolume(Math.round(value))
                }
            }
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 12
            anchors.bottomMargin: 4
            height: 22
            spacing: 16

            Text {
                objectName: "filenameLabel"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: PlayerTheme.textColor
                text: playerCtrl.mediaName
                font: PlayerTheme.textFont
                renderType: Text.QtRendering
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                maximumLineCount: 1
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                objectName: "progressLabel"
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                color: PlayerTheme.textColor
                text: playerCtrl.progressText.replace("/", " / ")
                font: PlayerTheme.textFont
                renderType: Text.QtRendering
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    onClosing: function(close) {
        videoArea.controller = null
        videoArea.visible = false
        close.accepted = true
    }

    Component.onDestruction: playerCtrl.cleanup()
}
