import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

TextField {
    id: textField
    property var model
    property string modelName

    property double max: 10000.0
    property double min: -10000.0

    width: 70
    height: 30
    font.pixelSize: 10
    color: "black"
    validator: DoubleValidator {}
    property string textString;
    text: model ? getText(textString) : text
    // cursorDelegate: CursorDelegate { }
    Rectangle {
        anchors.fill: parent
        border.width: 2
        border.color: textField.focus ?  '#186e02' : '#1a7a02'
        color: "white"
        Text {
            anchors.centerIn: parent
            font.pixelSize: 10
            color: "black" 
            text : textField.text
        }
    }

    function getText(value) {
        return value
    } 

    onEditingFinished: {
        if (model && modelName in model) {
            if (parseFloat(textField.text) > max) {
                model[modelName] = max;
                textString = max.toString();
                console.log("max");
                console.log("set " + modelName + " to " + model[modelName]);
                return;
            }
            if (parseFloat(textField.text) < min) {
                model[modelName] = min;
                textString = min.toString();
                console.log("min");
                return;
            }
            model[modelName] = parseFloat(textField.text);
            textString = model[modelName].toString();
            console.log("set " + modelName + " to " + model[modelName]);
        }
    }
}