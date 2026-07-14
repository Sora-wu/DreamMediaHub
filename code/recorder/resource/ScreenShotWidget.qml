import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    id: root

    readonly property int borderMargin: 3
    readonly property color tranparentColor: "transparent"       //透明色
    readonly property color blurryColor: Qt.rgba(0,0,0,0.3)      //模糊色
    property color selectBorderColor: "green"                    //边框色

    property string filePath

    signal  sigClose()
    signal regionAccepted(int x, int y, int w, int h)

    color: tranparentColor
    visibility: Qt.WindowFullScreen
    flags: Qt.FramelessWindowHint

    //四个阴影区域，笼罩未选择区域
    //一个选择区域
    Rectangle{
        id: topRect
        anchors{
            top: parent.top
            left: parent.left
        }
        width: parent.width
        height: selectionRect.y
        color: root.blurryColor
    }

    Rectangle{
        id: leftRect
        anchors{
            top: topRect.bottom
            left: parent.left
        }
        width: selectionRect.x
        height: parent.height - topRect.height - bottomRect.height
        color: root.blurryColor
    }

    Rectangle{
        id: rightRect
        anchors{
            top: topRect.bottom
            right: parent.right
        }
        width: parent.width - leftRect.width - selectionRect.width
        height: leftRect.height
        color: root.blurryColor
    }

    Rectangle{
        id: bottomRect
        anchors{
            bottom: parent.bottom
            left: parent.left
        }
        width: parent.width
        height: parent.height - topRect.height - selectionRect.height
        color: root.blurryColor
    }

    MouseArea {
        id: mainMouseArea
        anchors.fill: parent
        property int startX: 0
        property int startY: 0
        property int endX: 0
        property int endY: 0

        onPressed: function (mouse){
            startX = mouse.x;
            startY = mouse.y;
            selectionRect.width = 0;
            selectionRect.height = 0;
            selectionRect.visible = true;
        }
        onPositionChanged: function (mouse){
            if (pressed) {
                endX = mouse.x;
                endY = mouse.y;
                selectionRect.width = Math.abs(endX - startX);
                selectionRect.height = Math.abs(endY - startY);
                selectionRect.x = Math.min(startX, endX);
                selectionRect.y = Math.min(startY, endY);

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }
        onReleased: function (mouse){
            selectionRect.updateStartAndEndPoint();
        }
    }

    Rectangle {
        id: selectionRect

        property var startPoint
        property var endPoint

        function updateStartAndEndPoint() {
            startPoint = Qt.point(x, y);
            endPoint = Qt.point(x + width, y + height);
        }

        visible: false
        border.color: root.selectBorderColor
        border.width: 1
        color: root.tranparentColor

        MouseArea {
            id: dragItem
            anchors.fill: parent
            anchors.margins: 12 * 2
            cursorShape: Qt.SizeAllCursor
            drag.target: parent
            onPositionChanged: {
                selectionRect.updateStartAndEndPoint();
            }
        }

        Item{
            id: resizeBorderItem
            anchors.centerIn: parent
            width: parent.width + root.borderMargin * 2
            height: parent.height + root.borderMargin * 2
        }

        //LeftTop
        CusDragRect{
            id: dragLeftTop
            anchors{
                left: resizeBorderItem.left
                top: resizeBorderItem.top
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posLeftTop

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.endPoint.x);
                selectionRect.y = Math.min(point.y, selectionRect.endPoint.y);
                selectionRect.width = Math.max(selectionRect.endPoint.x, point.x)  - selectionRect.x;
                selectionRect.height = Math.max(selectionRect.endPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //LeftBottom
        CusDragRect{
            id: dragLeftBottom
            anchors{
                left: resizeBorderItem.left
                bottom: resizeBorderItem.bottom
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posLeftBottom

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.endPoint.x);
                selectionRect.y = Math.min(point.y, selectionRect.startPoint.y);
                selectionRect.width = Math.max(selectionRect.endPoint.x, point.x)  - selectionRect.x;
                selectionRect.height = Math.max(selectionRect.startPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //RightTop
        CusDragRect{
            id: dragRightTop
            anchors{
                right: resizeBorderItem.right
                top: resizeBorderItem.top
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posRightTop

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.startPoint.x);
                selectionRect.y = Math.min(point.y, selectionRect.endPoint.y);
                selectionRect.width = Math.max(selectionRect.startPoint.x, point.x)  - selectionRect.x;
                selectionRect.height = Math.max(selectionRect.endPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //RightBottom
        CusDragRect{
            id: dragRightBottom
            anchors{
                right: resizeBorderItem.right
                bottom: resizeBorderItem.bottom
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posRightBottom

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.startPoint.x);
                selectionRect.y = Math.min(point.y, selectionRect.startPoint.y);
                selectionRect.width = Math.max(selectionRect.startPoint.x, point.x)  - selectionRect.x;
                selectionRect.height = Math.max(selectionRect.startPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //Top
        CusDragRect{
            id: dragTop
            anchors{
                top: resizeBorderItem.top
                horizontalCenter: resizeBorderItem.horizontalCenter
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posTop

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.y = Math.min(point.y, selectionRect.endPoint.y);
                selectionRect.height = Math.max(selectionRect.endPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //Bottom
        CusDragRect{
            id: dragBottom
            anchors{
                bottom: resizeBorderItem.bottom
                horizontalCenter: resizeBorderItem.horizontalCenter
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posBottom

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.y = Math.min(point.y, selectionRect.startPoint.y);
                selectionRect.height = Math.max(selectionRect.startPoint.y, point.y) - selectionRect.y;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //Left
        CusDragRect{
            id: dragLeft
            anchors{
                left: resizeBorderItem.left
                verticalCenter: resizeBorderItem.verticalCenter
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posLeft

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.endPoint.x);
                selectionRect.width = Math.max(selectionRect.endPoint.x, point.x)  - selectionRect.x;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }

        //Right
        CusDragRect{
            id: dragRight
            anchors{
                right: resizeBorderItem.right
                verticalCenter: resizeBorderItem.verticalCenter
            }
            visible: selectionRect.visible
            callBackFunc : selectionRect.updateStartAndEndPoint
            posType: posLeft

            onSigPosChanged: function(mousePoint){
                var point = mapToGlobal(mousePoint);
                selectionRect.x = Math.min(point.x, selectionRect.startPoint.x);
                selectionRect.width = Math.max(selectionRect.startPoint.x, point.x)  - selectionRect.x;

                functionRect.areaUpdate(selectionRect.width, selectionRect.height);
            }
        }
    }

    Rectangle {
        id: functionRect
        width: 300
        height: 50
        anchors{
            top: selectionRect.bottom
            topMargin: 3
            right:selectionRect.right
        }

        function areaUpdate(sizeX, sizeY) {
            txtRecorderAreaSize.text = sizeX.toString() + " x " + sizeY.toString();
        }

        Rectangle {
            id: rctMenu
            width: 300
            height: 50
            color: "#101010"
            anchors.fill: parent

            Text {
                id: txtRecorderAreaSize
                color: "#ebebeb"
                text: "1920 x 1080"
                anchors.verticalCenter: parent.verticalCenter
                font.pointSize: 18
            }

            RecorderButton {
                id: btnConfirm
                width: 50
                color: "#1c1c1c"
                text: "确认"
                textColor: "#7a7a7a"
                anchors.left: txtRecorderAreaSize.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: 15
                anchors.topMargin: 0
                anchors.bottomMargin: 0

                onClicked: {
                    root.selectBorderColor = "red";
                    root.regionAccepted(selectionRect.x, selectionRect.y, selectionRect.width, selectionRect.height);
                    root.close();
                }
            }

            RecorderButton {
                id: btnCancel
                width: 50
                color: "#1c1c1c"
                text: "取消"
                textColor: "#7a7a7a"
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.rightMargin: 0
                anchors.topMargin: 0
                anchors.bottomMargin: 0

                onClicked: {
                    print("close");
                    g_recorderHeler.endRecord();
                    root.close()
                }
            }
        }
    }
}
