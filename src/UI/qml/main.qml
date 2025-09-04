import QtQuick 2.15
import QtQuick.Controls 2.15

import VulkanApp 1.0

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "Simple QML Application"

    VulkanItem {
        id: vulkanItem
        anchors.fill: parent
        anchors.margins: 10

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "white"
            border.width: 2
        }
    }
}
