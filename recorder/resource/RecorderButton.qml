import QtQuick 2.3

Rectangle {
    id: self
    width: 150
    height: 100
    color: "#3b3b3b"

    property string text: "test"
    property color textColor

    signal clicked()

    Text {
        anchors.centerIn: parent
        text: self.text
        color: self.textColor
        font.pointSize: 15
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            self.clicked();
        }
    }
}
