import QtQuick 2.15
import QtQuick.Controls 2.15

import VulkanApp 1.0

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "Simple QML Application"
    topPadding: 0
    leftPadding: 0
    bottomPadding: 0
    rightPadding: 0

    VulkanItem {
        id: vulkanItem
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "white"
            border.width: 2
        }
    }
}
