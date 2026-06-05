import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Window {
    width: 1280
    height: 720
    visible: true
    color: "#262626"
    minimumHeight: 720
    minimumWidth: 1280
    title: qsTr("DreamPlayer")

    PlayerController {
        id: playerCtrl
    }

    Rectangle {
        id: buttomBar
        height: 55
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "#3b4252"

        Column {
            id: columnLayout
            anchors.fill: parent
            spacing: 5

            RowLayout {
                id: controlLayout
                height: 25
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                spacing: 15

                RowLayout {
                    id: playLayout
                    height: 25
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    spacing: 15

                    Button {
                        id: btnPlayPause
                        icon.source: playerCtrl.isPlaying ? "qrc:/Dream/play/resource/image/pause.png"
                                                              : "qrc:/Dream/play/resource/image/play.png"
                        flat: true

                        onClicked: {
                            playerCtrl.togglePlay();
                        }
                    }

                    Button {
                        id: btnPrevious
                        icon.source: "qrc:/Dream/play/resource/image/previous.png"
                        flat: true
                    }

                    Button {
                        id: btnNext
                        icon.source: "qrc:/Dream/play/resource/image/next.png"
                        flat: true
                    }

                    ProcessBar {
                        id: videoBar
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        maxValue: playerCtrl.duration

                        onSliderRelease: function(value) {
                            playerCtrl.seek(value)
                        }
                    }
                }

                RowLayout {
                    id: volumeLayout
                    height: 25
                    Layout.fillHeight: true
                    spacing: 8
                    Layout.fillWidth: true

                    Button {
                        id: btnVolume
                        icon.source: "qrc:/Dream/play/resource/image/volume.png"
                        flat: true
                    }

                    ProcessBar {
                        id: volumeBar
                        currentVal: 100
                        progress: 1
                        maxValue: 100
                        Layout.preferredWidth: 150
                        showCurrentValue: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                }
            }

            RowLayout {
                id: rowLayout
                y: 25
                height: 25
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 0
                anchors.rightMargin: 0

                Text {
                    id: txtFilename
                    color: "#ffffff"
                    text: "111.mp4"
                    font.pixelSize: 20
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillWidth: true
                    style: Text.Outline
                    Layout.leftMargin: 0
                }

                Text {
                    id: txtProgress
                    color: "#ffffff"
                    text: "114:51/419:19"
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    Layout.rightMargin: 0
                    Layout.fillWidth: true
                    style: Text.Outline
                }
            }
        }
    }

    VideoArea {
        id: videoArea
        objectName: "videoArea"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttomBar.top

        controller: playerCtrl
    }

}
