import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import "Library"
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
            height: 60
            color: "#2c5037"
            border.color: "#16291c"
            border.width: 2
            RowLayout {
                anchors.fill: parent
                GeoButton {
                    text: "addLine"
                    onClicked: {
                        mainWindow.addLine();
                        vulkanItem.addingLine();
                    }
                }
                GeoDoubleBoxWithLable {
                    model: mainWindow
                    modelName: "x1"
                    labelText: "X1"
                }
                GeoDoubleBoxWithLable {
                    model: mainWindow
                    modelName: "y1"
                    labelText: "Y1"
                }
                GeoDoubleBoxWithLable {
                    model: mainWindow
                    modelName: "x2"
                    labelText: "X2"
                }
                GeoDoubleBoxWithLable {
                    model: mainWindow
                    modelName: "y2"
                    labelText: "Y2"
                }
                GeoButton {
                    text: "addLine"
                    onClicked: {
                        vulkanItem.addingLineWithCoordinates(mainWindow.x1, mainWindow.y1, mainWindow.x2, mainWindow.y2);
                        mainWindow.addLineWithCoordinates();
                        console.log(mainWindow.y1);
                    }
                }
                GeoButton {
                    text: "addLine"
                    onClicked: {
                        mainWindow.addLine();
                        vulkanItem.addingLine();
                    }
                }
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
                anchors.margins: 0
            }
            anchors.margins: 0
            // Component.onCompleted: {
            //     vulkanItem.addingLine.connect(mainWindow.lineSignal);
            // } 
        }
    }
}
