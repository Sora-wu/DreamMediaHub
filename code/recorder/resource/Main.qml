import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

Window {
    id: recorder
    width: 640
    height: 400
    visible: true
    color: "#101010"

    property bool isRecording: false

    // 存储选区坐标的属性
    property int areaX: 0
    property int areaY: 0
    property int areaW: 1920
    property int areaH: 1080

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

            // 第一行：Url
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

            // 第二行：恒定码率
            RowLayout {
                Layout.fillWidth: true
                Label { text: "开启恒定码率:"; color: "#b1b1b1"; font.pixelSize: 18 }
                CheckBox {
                    id: cbrCheckBox
                    enabled: !recorder.isRecording
                }
            }

            // 第三行：坐标及宽高
            RowLayout {
                id: coordsLayout
                Layout.fillWidth: true
                spacing: 10
                enabled: !recorder.isRecording // 录制时禁用

                // 定义一个复用的输入框组件样式
                component CoordInput : TextField {
                    Layout.preferredWidth: 60
                    color: "#b1b1b1"
                    validator: IntValidator { bottom: 0 }
                    background: Rectangle { color: "#222"; radius: 4; border.color: "#444" }
                }

                Label { text: "X:"; color: "#b1b1b1" }
                CoordInput { id: xInput; text: recorder.areaX.toString(); onTextChanged: recorder.areaX = parseInt(text) }

                Label { text: "Y:"; color: "#b1b1b1" }
                CoordInput { id: yInput; text: recorder.areaY.toString(); onTextChanged: recorder.areaY = parseInt(text) }

                Label { text: "W:"; color: "#b1b1b1" }
                CoordInput { id: wInput; text: recorder.areaW.toString(); onTextChanged: recorder.areaW = parseInt(text) }

                Label { text: "H:"; color: "#b1b1b1" }
                CoordInput { id: hInput; text: recorder.areaH.toString(); onTextChanged: recorder.areaH = parseInt(text) }
            }

            Item { Layout.fillHeight: true }

            // 第四行：功能按钮（手动选区 + 开始录制）
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                Button {
                    id: selectAreaButton
                    text: "手动选区"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    enabled: !recorder.isRecording
                    onClicked: {
                        screenShotWidgetLoader.source = "ScreenShotWidget.qml"
                    }
                }

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
                            console.log("开始录制:", recorder.areaX, recorder.areaY, recorder.areaW, recorder.areaH);
                            // 这里调用你的 C++ 录制接口，传入 recorder.areaX 等属性
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
}