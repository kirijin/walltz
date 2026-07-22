import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    property string title
    property bool expanded: true
    property bool collapsible: true
    property real headerImplicitHeight: Kirigami.Units.gridUnit * 2
    default property alias content: contentColumn.data

    spacing: 0

    // ── Header ──
    Item {
        id: headerArea
        Layout.fillWidth: true
        implicitHeight: headerText.implicitHeight + Kirigami.Units.smallSpacing * 2

        readonly property bool _hovered: headerMouse.containsMouse || headerMouse.pressed

        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius
            color: headerArea._hovered
                   ? Kirigami.Theme.hoverColor
                   : "transparent"
            Behavior on color { ColorAnimation { duration: 100 } }
        }

        RowLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing

            Controls.Label {
                text: root.expanded ? "▼" : "▶"
                font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                color: Kirigami.Theme.textColor
                opacity: 0.6
            }

            Controls.Label {
                id: headerText
                text: root.title
                font.weight: Font.DemiBold
                color: Kirigami.Theme.textColor
            }
        }

        MouseArea {
            id: headerMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.collapsible) {
                    root.expanded = !root.expanded;
                }
            }
        }
    }

    // ── Content with collapse animation ──
    Item {
        id: contentWrapper
        Layout.fillWidth: true
        clip: true

        implicitHeight: contentColumn.implicitHeight + (contentColumn.implicitHeight > 0 ? Kirigami.Units.smallSpacing : 0)
        height: root.expanded ? implicitHeight : 0
        visible: root.expanded || height > 0
        enabled: root.expanded

        Behavior on height {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        ColumnLayout {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: Kirigami.Units.smallSpacing
        }
    }
}
