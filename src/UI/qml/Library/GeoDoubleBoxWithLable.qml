import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Rectangle {
    id: geoDoubleBox
    property string labelText: "Label"

    property alias model: innerDoubleBox.model
    property alias modelName: innerDoubleBox.modelName
    property alias min: innerDoubleBox.min
    property alias max: innerDoubleBox.max
    color: "transparent"
    width: 70
    height: 50

    ColumnLayout {
        anchors.fill: parent
        spacing: 2
        Text {
            text: geoDoubleBox.labelText
            font.pixelSize: 10
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }
        GeoDoubleBox {
            id: innerDoubleBox
            model: geoDoubleBox.model
            modelName: geoDoubleBox.modelName
            min: geoDoubleBox.min
            max: geoDoubleBox.max
            Layout.alignment: Qt.AlignHCenter
        }
    }
}