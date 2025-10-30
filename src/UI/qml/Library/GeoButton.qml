import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Button {
    id: button
    width: 70
    height: 30
    contentItem: Rectangle {
        id: rect
        anchors.fill: parent
        border.width: 2
        border.color: button.down ?  '#186e02' : '#1a7a02'
        color: button.down ? '#26a406' : '#2dcb05'
        Text {
            anchors.centerIn: parent
            font.pixelSize: 10
            color: "white" 
            text : parent.parent.text
        }
    }
}