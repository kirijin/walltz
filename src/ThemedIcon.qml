import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Item {
    property alias source: icon.source
    property color colorType: Kirigami.Theme.textColor
    property int iconSize: 24

    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: icon
        anchors.centerIn: parent
        width: iconSize
        height: iconSize
        fillMode: Image.PreserveAspectFit
        visible: false
    }

    MultiEffect {
        source: icon
        anchors.centerIn: parent
        width: iconSize
        height: iconSize
        colorization: 1.0
        colorizationColor: parent.colorType
    }
}
