import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15


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
    ColumnLayout {
        anchors.fill: parent
        Rectangle {
            visible:true
            Layout.fillWidth: true
            height: 50
            color: "#2c3e50"
            Text {
                anchors.centerIn: parent
                text: "GeoCAD Vulkan Application"
                color: "white"
                font.pixelSize: 20
            }
        }
        spacing: 0
        VulkanItem {
            id: vulkanItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                color: '#9db0c4'
                border.color: "white"
                border.width: 2
            }
        }
    }
}
