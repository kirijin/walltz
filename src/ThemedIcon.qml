import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Item {
    property alias source: icon.source
    property color colorType: Kirigami.Theme.textColor
    property int iconSize: 18

    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: icon
        anchors.fill: parent
        sourceSize.width: parent.iconSize
        sourceSize.height: parent.iconSize
        fillMode: Image.PreserveAspectFit
        layer.enabled: true
        layer.effect: MultiEffect {
            colorizationEnabled: true
            colorizationColor: icon.parent.colorType
            colorizationAmount: 1.0
        }
    }
}
