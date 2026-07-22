import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Item {
    property alias source: icon.source
    property color colorType: Kirigami.Theme.textColor
    property int iconSize: 20

    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: icon
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: false
    }

    MultiEffect {
        source: icon
        anchors.fill: parent
        colorization: 1.0
        colorizationColor: parent.colorType
    }
}
