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

    function urlToLocalPath(url) {
        let path = url.toString();
        if (path.startsWith("file://")) {
            path = path.substring(7);
        }

        // 兼容 Windows 系统
        if (path.length > 2 && path.charAt(0) === '/' && path.charAt(2) === ':') {
            path = path.substring(1);
        }
        return path;
    }

    function isLocalPath(path) {
        if (!path || typeof path !== "string") return false

        var p = path.replace(/\\/g, "/")
        var match = p.match(/^([a-zA-Z][a-zA-Z0-9+\-.]*):/)
        var scheme = match ? match[1].toLowerCase() : ""

        if (scheme === "") return true
        if (scheme === "file" || scheme === "qrc") return true

        var remoteSchemes = ["http", "https", "ftp", "rtmp", "rtsp", "mms", "udp", "tcp"]
        if (remoteSchemes.indexOf(scheme) !== -1) return false

        // 其他不认识的 scheme，保守视为远程
        return false
    }

    function generateFileName(path) {
        let folderStr = path;

        if (!isLocalPath(folderStr)) {
            // 如果不是本地路径，可能是流媒体，直接返回
            return folderStr;
        }

        // 如果是本地文件，则加上文件名
        folderStr = urlToLocalPath(folderStr);
        if (!folderStr.endsWith("/")) {
            folderStr += "/";
        }

        // 格式化当前时间
        let timestamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd_hh-mm-ss");
        return folderStr + "DreamRecorder_" + timestamp + ".mp4";
    }

    Settings {
        id: appSettings
        property string lastFolder: ""
    }

    RecorderController {
        id: recorderCtrl
    }

    FolderDialog {
        id: fd
        acceptLabel: qsTr("确定")
        rejectLabel: qsTr("取消")
        currentFolder: appSettings.lastFolder ? appSettings.lastFolder : StandardPaths.writableLocation(StandardPaths.MoviesLocation)
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
                            let path = urlInput.text;
                            appSettings.lastFolder = path;
                            path = generateFileName(path);
                            console.log("url:", path);
                            recorderCtrl.start(path);
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