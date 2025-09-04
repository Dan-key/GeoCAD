import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "Simple QML Application"

    Button {
        text: "Click Me"
        anchors.centerIn: parent
        onClicked: {
            messageLabel.text = "Hello, QML!"
        }
    }

    Label {
        id: messageLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        text: ""
    }
}
