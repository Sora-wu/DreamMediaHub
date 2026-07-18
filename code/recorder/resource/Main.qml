import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

Window {
    id: recorder
    width: 640
    height: 320
    visible: true
    color: "#101010"

    property bool isRecording: false

    RecorderController {
        id: recorderCtrl
    }

    FolderDialog {
        id: fd
        acceptLabel: qsTr("确定")
        rejectLabel: qsTr("取消")
        currentFolder: "/home/sora/Videos/"
        onAccepted: {
            fd.currentFolder = selectedFolder;
            urlInput.text = selectedFolder
        }
    }

    Rectangle {
        id: content
        color: "#101010"
        anchors.fill: parent
        anchors.margins: 40

        ColumnLayout {
            id: columnLayout
            anchors.fill: parent
            spacing: 15

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Label { text: "Url:"; color: "#b1b1b1"; font.pixelSize: 18 }
                TextField {
                    id: urlInput
                    Layout.fillWidth: true
                    text: fd.currentFolder
                    enabled: !recorder.isRecording
                    background: Rectangle { color: "#222"; radius: 4; border.color: "#444" }
                    color: "#b1b1b1"
                }
                Button {
                    text: "选择目录"
                    enabled: !recorder.isRecording
                    onClicked: fd.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: "开启恒定码率:"; color: "#b1b1b1"; font.pixelSize: 18 }
                CheckBox {
                    id: cbrCheckBox
                    enabled: !recorder.isRecording
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                Button {
                    id: recordButton
                    text: recorder.isRecording ? "停止录制" : "开始录制"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40

                    contentItem: Text {
                        text: parent.text
                        color: "#fff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true
                    }

                    background: Rectangle {
                        color: recorder.isRecording ? "#e74c3c" : "#2ecc71"
                        radius: 4
                    }

                    onClicked: {
                        recorder.isRecording = !recorder.isRecording;
                        if (recorder.isRecording) {
                            recorderCtrl.start(fd.currentFolder + "recording.mp4",)
                        }
                        else {
                            recorderCtrl.stop();
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: screenShotWidgetLoader
        onLoaded: {
            item.filePath = fd.currentFolder;

            // 关键：连接选区完成信号
            item.regionAccepted.connect(function(x, y, w, h) {
                recorder.areaX = x;
                recorder.areaY = y;
                recorder.areaW = w;
                recorder.areaH = h;
                // 自动更新输入框的文本
                xInput.text = x.toString();
                yInput.text = y.toString();
                wInput.text = w.toString();
                hInput.text = h.toString();
            });

            item.closing.connect(function () {
                screenShotWidgetLoader.source = "";
            })
        }
    }

    Component.onDestruction: {
        if (recorder.isRecording) {
            recorderCtrl.stop();
        }
    }
}