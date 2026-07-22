import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.walltz.processor 1.0

Kirigami.ApplicationWindow {
    id: root

    width: 640
    height: 720
    minimumWidth: 640
    minimumHeight: 600

    title: i18nc("@title:window", "Walltz")

    pageStack.initialPage: Kirigami.Page {
        id: mainPage

        title: i18n("Walltz")

        actions: [
            Kirigami.Action {
                text: i18n("Walltz it")
                icon.name: "document-save"
                displayHint: Kirigami.DisplayHint.KeepVisible
                enabled: dropArea.fileCount > 0
                         && widthInput.length > 0 && heightInput.length > 0
                         && !processor.busy
                onTriggered: processor.processQueue(dropArea.filePaths)
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                shortcut: "Ctrl+Q"
                onTriggered: Qt.quit()
            }
        ]

        Kirigami.Theme.colorSet: Kirigami.Theme.View

        Component.onCompleted: {
            processor.detectScreenSize();
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing
            anchors.margins: Kirigami.Units.largeSpacing

            // === Drop area / preview ===
            Item {
                id: previewBox
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
                Layout.minimumHeight: 100

                readonly property double _ar: processor.targetWidth / Math.max(1, processor.targetHeight)
                Layout.preferredHeight: root.height * 0.40

                readonly property color _canvasColor: Kirigami.Theme.colorScheme === Kirigami.Theme.Dark
                    ? Qt.darker(Kirigami.Theme.backgroundColor, 2.5)
                    : Qt.darker(Kirigami.Theme.backgroundColor, 3.0)
                Rectangle {
                    id: dropZone
                    anchors.centerIn: parent
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: Qt.rgba(0, 0, 0, 0.4)
                        shadowBlur: 16
                        shadowHorizontalOffset: 0
                        shadowVerticalOffset: 4
                        shadowOpacity: 0.7
                    }

                    readonly property double _scale: Math.min(
                        parent.width / previewBox._ar,
                        parent.height
                    )
                    width: previewBox._ar * _scale
                    height: _scale

                    radius: Kirigami.Units.cornerRadius
                    color: dropArea.containsDrag ? Kirigami.Theme.highlightColor
                          : (dropArea.fileCount === 0 ? "transparent" : previewBox._canvasColor)
                    border.color: dropArea.fileCount === 0 ? Kirigami.Theme.disabledTextColor : "transparent"
                    border.width: dropArea.fileCount === 0 ? 1 : 0
                    Behavior on color { ColorAnimation { duration: 150 } }

                    DropArea {
                        id: dropArea
                        anchors.fill: parent
                        property int fileCount: 0
                        property var filePaths: []
                        property var fileList: []  // [{path, previewUrl}]

                        onDropped: function (drop) {
                            if (drop.hasUrls && drop.urls.length > 0) {
                                var entries = [];
                                var paths = [];
                                for (var i = 0; i < drop.urls.length; ++i) {
                                    var urlStr = drop.urls[i].toString();
                                    if (urlStr.startsWith("file://"))
                                        urlStr = decodeURIComponent(urlStr.substring(7));
                                    if (urlStr.length > 0) {
                                        paths.push(urlStr);
                                        var pv = processor.generatePreview(urlStr);
                                        entries.push({path: urlStr, previewUrl: pv});
                                    }
                                }
                                fileList = entries;
                                filePaths = paths;
                                fileCount = paths.length;
                                previewList.model = fileList;
                                if (fileList.length > 0)
                                    previewA.source = fileList[0].previewUrl;
                                drop.accept();
                            }
                        }
                        onEntered: function (drag) { if (drag.hasUrls) drag.accept(); }
                    }

                    Item {
                        id: previewContainer
                        anchors.fill: parent
                        visible: dropArea.fileCount > 0

                        Image {
                            id: previewA
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            asynchronous: true
                            opacity: 1.0
                        }

                        Image {
                            id: previewB
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            asynchronous: true
                            opacity: 0.0
                        }

                        SequentialAnimation {
                            id: fadeAnim
                            NumberAnimation { target: previewB; property: "opacity"; to: 1.0; duration: 200 }
                            ScriptAction {
                                script: {
                                    // Keep A as the active layer, recycle B for next crossfade
                                    previewA.source = previewB.source;
                                    previewB.opacity = 0.0;
                                    previewB.source = "";
                                }
                            }
                        }
                    }

                    Item {
                        anchors.centerIn: parent
                        visible: dropArea.fileCount === 0 && !dropArea.containsDrag

                        readonly property var _frames: [
                            "\u280B", "\u2819", "\u2839", "\u2838", "\u283C",
                            "\u2834", "\u2826", "\u2827", "\u2807", "\u280F"
                        ]
                        property int _frame: 0

                        Timer {
                            interval: 100; repeat: true
                            running: parent.visible
                            onTriggered: {
                                parent._frame = (parent._frame + 1) % parent._frames.length
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.smallSpacing

                            Controls.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: parent.parent._frames[parent.parent._frame]
                                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 2
                                color: Kirigami.Theme.disabledTextColor
                            }
                            Controls.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: i18n("Drop image(s) here")
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    Controls.Label {
                        id: moreFilesLabel
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Kirigami.Units.smallSpacing
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: dropArea.fileCount > 1
                        text: i18n("+ %1 more files", dropArea.fileCount - 1)
                        color: Kirigami.Theme.disabledTextColor
                    }

                    // ── Processing overlay ──
                    Rectangle {
                        anchors.fill: parent
                        visible: processor.busy
                        color: Qt.rgba(0, 0, 0, 0.55)
                        radius: parent.radius
                        z: 10

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                text: i18n("Processing\u2026")
                                color: "white"
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Controls.ProgressBar {
                                indeterminate: true
                                Layout.fillWidth: true
                                Layout.preferredWidth: 200
                            }
                        }
                    }
                }
            }

            // === Resolution row ===
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Item { Layout.fillWidth: true }

                // Braille processing indicator
                Controls.Label {
                    id: processingIndicator
                    visible: processor.busy
                    text: "\u28BE"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize
                    Layout.alignment: Qt.AlignVCenter
                    Timer {
                        interval: 150
                        running: processor.busy
                        repeat: true
                        property int frame: 0
                        readonly property var frames: [
                            "\u28B8", "\u28B9", "\u28BA", "\u28BB",
                            "\u28BC", "\u28BD", "\u28BE", "\u28BF"
                        ]
                        onTriggered: {
                            frame = (frame + 1) % frames.length;
                            parent.text = frames[frame];
                        }
                    }
                }

                Controls.TextField {
                    id: widthInput
                    Layout.preferredWidth: 80
                    inputMethodHints: Qt.ImhDigitsOnly
                    placeholderText: i18n("Width")
                    text: processor.targetWidth
                    validator: IntValidator { bottom: 1; top: 14999 }
                    onTextChanged: {
                        var v = parseInt(text);
                        if (!isNaN(v) && v > 0) processor.targetWidth = v;
                    }
                }
                Controls.ToolButton {
                    id: swapBtn
                    icon.name: "swap-panels"
                    display: Controls.AbstractButton.IconOnly
                    hoverEnabled: true
                    Controls.ToolTip.text: i18n("Swap width and height")
                    Controls.ToolTip.visible: swapBtn.hovered
                    Controls.ToolTip.delay: 400
                    onClicked: {
                        var tmp = processor.targetWidth;
                        processor.targetWidth = processor.targetHeight;
                        processor.targetHeight = tmp;
                        processor.aspectMode = 0;
                    }
                }
                Controls.TextField {
                    id: heightInput
                    Layout.preferredWidth: 80
                    inputMethodHints: Qt.ImhDigitsOnly
                    placeholderText: i18n("Height")
                    text: processor.targetHeight
                    validator: IntValidator { bottom: 1; top: 14999 }
                    onTextChanged: {
                        var v = parseInt(text);
                        if (!isNaN(v) && v > 0) processor.targetHeight = v;
                    }
                }

                Controls.ToolButton {
                    id: resetResBtn
                    icon.name: "video-display-symbolic"
                    text: i18n("Detect")
                    display: Controls.AbstractButton.IconOnly
                    hoverEnabled: true
                    Controls.ToolTip.text: i18n("Reset to screen resolution (%1\u00D7%2)",
                                       processor.screenWidth, processor.screenHeight)
                    Controls.ToolTip.visible: resetResBtn.hovered
                    onClicked: {
                        processor.aspectMode = 0;
                        processor.detectScreenSize();
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // === Aspect ratio preset ===
            Controls.ButtonGroup { id: ratioGroup }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Item { Layout.fillWidth: true }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    // Generic zone
                    Controls.Button {
                        text: i18n("Free")
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 0
                        onClicked: processor.aspectMode = 0
                    }
                    Controls.Button {
                        text: "1:1"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 1
                        onClicked: processor.aspectMode = 1
                    }
                    Controls.Button {
                        text: "4:3"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 2
                        onClicked: processor.aspectMode = 2
                    }
                    Controls.Button {
                        text: "16:10"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 4
                        onClicked: processor.aspectMode = 4
                    }

                    // Wide zone
                    Controls.Button {
                        text: "16:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 3
                        onClicked: processor.aspectMode = 3
                    }
                    Controls.Button {
                        text: "21:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 5
                        onClicked: processor.aspectMode = 5
                    }
                    Controls.Button {
                        text: "32:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        highlighted: checked
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 6
                        onClicked: processor.aspectMode = 6
                    }
                }

                Item { Layout.fillWidth: true }
            }
            // ── Top bar: mode + mood (always visible) ──
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                // Blur | Colour toggle
                Controls.ButtonGroup { id: modeGroup }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Controls.Button {
                        text: i18n("Blur")
                        checkable: true
                        flat: true
                        implicitWidth: Kirigami.Units.gridUnit * 7
                        highlighted: checked
                        checked: processor.blurMode
                        onClicked: processor.blurMode = true
                        Controls.ButtonGroup.group: modeGroup
                    }
                    Controls.Button {
                        text: i18n("Colour")
                        checkable: true
                        flat: true
                        implicitWidth: Kirigami.Units.gridUnit * 7
                        highlighted: checked
                        checked: !processor.blurMode
                        onClicked: processor.blurMode = false
                        Controls.ButtonGroup.group: modeGroup
                    }

                    Item { Layout.fillWidth: true }
                }

                // Sub-style: Solid / Gradient / Auto (when Colour)
                Controls.ButtonGroup { id: fillGroup }

                RowLayout {
                    visible: !processor.blurMode
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Controls.Button {
                        text: i18n("Colour")
                        checkable: true
                        flat: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        highlighted: checked
                        checked: processor.bgGradientStyle === 0
                        onClicked: processor.bgGradientStyle = 0
                        Controls.ButtonGroup.group: fillGroup
                    }
                    Controls.Button {
                        text: i18n("Gradient")
                        checkable: true
                        flat: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        highlighted: checked
                        checked: processor.bgGradientStyle === 1
                        onClicked: processor.bgGradientStyle = 1
                        Controls.ButtonGroup.group: fillGroup
                    }
                    Controls.Button {
                        text: i18n("Auto")
                        checkable: true
                        flat: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        highlighted: checked
                        checked: processor.bgGradientStyle === 2
                        onClicked: processor.bgGradientStyle = 2
                        Controls.ButtonGroup.group: fillGroup
                    }

                    Item { Layout.fillWidth: true }
                }

                // Mood palette buttons — only when Fill=Auto
                Controls.ButtonGroup { id: moodGroup }

                Timer {
                    id: moodPreviewTimer
                    interval: 200
                    onTriggered: {
                        if (dropArea.fileCount > 0 && dropArea.filePaths.length > 0) {
                            var url = processor.generatePreview(dropArea.filePaths[0])
                            if (url.length > 0)
                                crossfadePreview(url)
                        }
                    }
                }

                RowLayout {
                    visible: !processor.blurMode && processor.bgGradientStyle === 2
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Repeater {
                        model: 6
                        delegate: Controls.Button {
                            text: processor.moodName(index)
                            checkable: true
                            flat: true
                            highlighted: checked
                            checked: !processor.useV2 && processor.autoMood === index
                            onClicked: {
                                processor.autoMood = index
                                processor.useV2 = false
                                moodPreviewTimer.restart()
                            }
                            implicitWidth: Kirigami.Units.gridUnit * 5
                            Controls.ButtonGroup.group: moodGroup
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    visible: !processor.blurMode && processor.bgGradientStyle === 2
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Repeater {
                        model: 6
                        delegate: Controls.Button {
                            text: processor.moodNameV2(index)
                            checkable: true
                            flat: true
                            highlighted: checked
                            checked: processor.useV2 && processor.autoMood === index
                            onClicked: {
                                processor.autoMood = index
                                processor.useV2 = true
                                moodPreviewTimer.restart()
                            }
                            implicitWidth: Kirigami.Units.gridUnit * 5
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // ── Content + sliders ──
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                // Solid colour chooser (when Colour + Solid)
                RowLayout {
                    visible: !processor.blurMode && processor.bgGradientStyle === 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    // Auto — star indicator
                    Rectangle {
                        id: autoColorRect
                        implicitWidth: 28; implicitHeight: 28
                        radius: 4
                        border.width: processor.autoColor ? 2 : 1
                        border.color: processor.autoColor
                                       ? Kirigami.Theme.highlightColor
                                       : Kirigami.Theme.disabledTextColor
                        color: processor.autoColor && dropArea.fileCount > 0
                               ? processor.backgroundColor
                               : "transparent"

                        Controls.Label {
                            anchors.centerIn: parent
                            text: "\u2605"
                            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize
                            color: autoColorRect.border.color
                        }

                        Controls.Button {
                            anchors.fill: parent
                            opacity: 0
                            onClicked: processor.autoColor = true
                        }
                    }

                    // Visual spacer
                    Item {
                        implicitWidth: Kirigami.Units.largeSpacing
                        implicitHeight: 1
                    }

                    // Preset swatches
                    Repeater {
                        model: [
                            "#ff6b6b","#f0932b","#f9ca24","#6ab04c",
                            "#22a6b3","#4834d4","#be2edd","#666666","#000000"
                        ]

                        Rectangle {
                            required property string modelData

                            implicitWidth: 28; implicitHeight: 28
                            radius: 4
                            border.width: (!processor.autoColor
                                           && processor.backgroundColor.toString().toUpperCase() === modelData.toUpperCase())
                                          ? 2 : 1
                            border.color: (!processor.autoColor
                                           && processor.backgroundColor.toString().toUpperCase() === modelData.toUpperCase())
                                          ? Kirigami.Theme.highlightColor
                                          : Kirigami.Theme.textColor
                            color: modelData

                            Controls.Button {
                                anchors.fill: parent
                                opacity: 0
                                onClicked: {
                                    processor.autoColor = false
                                    processor.backgroundColor = modelData
                                }
                            }
                        }
                    }

                    // More button
                    Controls.ToolButton {
                        text: "+"
                        implicitWidth: 28; implicitHeight: 28
                        font.bold: true
                        onClicked: colorDialog.open()
                        Controls.ToolTip.text: i18n("More colors\u2026")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                    }

                    Item { Layout.fillWidth: true }
                }

                // Gradient preset picker (when Colour + Gradient)
                RowLayout {
                    visible: !processor.blurMode && processor.bgGradientStyle === 1
                    Layout.fillWidth: true
                    spacing: 0

                    Item { Layout.fillWidth: true }

                    GridLayout {
                        columns: 5
                        columnSpacing: Kirigami.Units.mediumSpacing
                        rowSpacing: Kirigami.Units.mediumSpacing

                        Repeater {
                            model: processor.gradientPresetCount()

                            Rectangle {
                                id: presetDelegate

                                required property int index

                                width: 56; height: 40
                                radius: Kirigami.Units.cornerRadius
                                border.width: processor.bgGradientPreset === index ? 2 : 1
                                border.color: processor.bgGradientPreset === index
                                               ? Kirigami.Theme.highlightColor
                                               : Kirigami.Theme.textColor

                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: processor.gradientPresetColor1(presetDelegate.index) }
                                    GradientStop { position: 1.0; color: processor.gradientPresetColor2(presetDelegate.index) }
                                }

                                Controls.Button {
                                    anchors.fill: parent
                                    opacity: 0
                                    onClicked: processor.bgGradientPreset = index
                                }

                                Controls.Label {
                                    anchors.bottom: parent.bottom
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottomMargin: 2
                                    text: String(presetDelegate.index + 1)
                                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                                    color: Kirigami.Theme.textColor
                                    style: Text.Outline
                                    styleColor: Kirigami.Theme.backgroundColor
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Vignette (always visible)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("V")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.vignetteStrength = 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 1.0; stepSize: 0.05
                        value: processor.vignetteStrength
                        Controls.ToolTip.text: i18n("%1%").arg(Math.round(processor.vignetteStrength * 100))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: {
                            processor.vignetteStrength = value
                            previewDebounce.restart()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // Grain (always visible)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("G")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.grainStrength = 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 1.0; stepSize: 0.05
                        value: processor.grainStrength
                        Controls.ToolTip.text: i18n("%1%").arg(Math.round(processor.grainStrength * 100))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: {
                            processor.grainStrength = value
                            previewDebounce.restart()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // Chromatic aberration (always visible)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("CA")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.caStrength = 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 1.0; stepSize: 0.05
                        value: processor.caStrength
                        Controls.ToolTip.text: i18n("%1%").arg(Math.round(processor.caStrength * 100))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: {
                            processor.caStrength = value
                            previewDebounce.restart()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                // Blur radius (when Blur selected)
                RowLayout {
                    visible: processor.blurMode
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Blur")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.blurRadius = 0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: blurSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 120; stepSize: 1
                        value: processor.blurRadius
                        Controls.ToolTip.text: processor.blurRadius === 0
                                      ? i18n("Auto")
                                      : i18n("%1 px").arg(processor.blurRadius)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: processor.blurRadius = value
                    }
                    Item { Layout.fillWidth: true }
                }

                // Saturation (when Blur selected)
                RowLayout {
                    visible: processor.blurMode
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Sat")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.saturationFactor = 1.8
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: satSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 30; stepSize: 1
                        value: processor.saturationFactor * 10
                        Controls.ToolTip.text: i18n("%1×").arg(processor.saturationFactor.toFixed(1))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: processor.saturationFactor = value / 10.0
                    }
                    Item { Layout.fillWidth: true }
                }

                // Background zoom (when Blur selected)
                RowLayout {
                    visible: processor.blurMode
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Zoom")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.bgZoom = 1.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: zoomSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 5; to: 30; stepSize: 1
                        value: Math.round(processor.bgZoom * 10)
                        Controls.ToolTip.text: i18n("%1%").arg(Math.round(processor.bgZoom * 100))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: processor.bgZoom = value / 10.0
                    }
                    Item { Layout.fillWidth: true }
                }

                // Background rotation (when Blur selected)
                RowLayout {
                    visible: processor.blurMode
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Rot")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.bgBlurAngle = 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: bgRotSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 360; stepSize: 1
                        value: processor.bgBlurAngle
                        Controls.ToolTip.text: i18n("%1°").arg(processor.bgBlurAngle)
                        Controls.ToolTip.visible: hovered
                        onMoved: processor.bgBlurAngle = value
                    }
                    Item { Layout.fillWidth: true }
                }

                // Gradient angle (when Colour + Gradient/Auto)
                RowLayout {
                    visible: !processor.blurMode && processor.bgGradientStyle > 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Angle")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.gradientAngle = 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: gradSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 360; stepSize: 1
                        value: processor.gradientAngle
                        Controls.ToolTip.text: i18n("%1°").arg(processor.gradientAngle)
                        Controls.ToolTip.visible: hovered
                        onMoved: processor.gradientAngle = value
                    }
                    Item { Layout.fillWidth: true }
                }

                // Photo frame (text button, like vignette/grain — slider at 0 = off)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }
                    Controls.Button {
                        text: i18n("Frame")
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        onClicked: {
                            processor.photoFrameWidth = 0
                            processor.photoFrame = false
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        id: frameWidthSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 25; stepSize: 1
                        value: processor.photoFrameWidth
                        Controls.ToolTip.text: processor.photoFrameWidth === 0
                                      ? i18n("Off")
                                      : i18n("%1 px").arg(processor.photoFrameWidth)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: {
                            processor.photoFrameWidth = value
                            if (value > 0) processor.photoFrame = true
                            previewDebounce.restart()
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                Controls.Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: i18n("Reset effects")
                    icon.name: "edit-undo"
                    onClicked: {
                        processor.vignetteStrength = 0.0
                        processor.grainStrength = 0.0
                        processor.blurRadius = 0
                        processor.saturationFactor = 1.8
                        processor.bgZoom = 1.0
                        processor.bgBlurAngle = 0.0
                        processor.gradientAngle = 0.0
                        processor.caStrength = 0.0
                        processor.photoFrameWidth = 0
                        processor.photoFrame = false
                        previewDebounce.restart()
                    }
                }

            }
            Controls.Label {
                visible: dropArea.fileCount > 1
                text: i18n("Files to process:")
                color: Kirigami.Theme.disabledTextColor
            }
            ListView {
                id: previewList
                visible: dropArea.fileCount > 1
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count * 40, 120)
                model: []
                delegate: RowLayout {
                    id: fileDelegate
                    width: parent ? parent.width : 0
                    spacing: Kirigami.Units.smallSpacing

                    required property var modelData

                    Image {
                        source: modelData ? modelData.previewUrl : ""
                        sourceSize.width: 32; sourceSize.height: 32
                        fillMode: Image.PreserveAspectFit
                        Layout.preferredWidth: 32; Layout.preferredHeight: 32
                        visible: status === Image.Ready
                    }
                    Controls.Label {
                        text: modelData ? modelData.path.toString().split("/").pop() : ""
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }
            }

            // === Status / InlineMessage ===
            Kirigami.InlineMessage {
                id: statusMessage
                Layout.fillWidth: true
                showCloseButton: true
                visible: false
            }

            Item { Layout.fillHeight: true }
        }
    }

    // ── Color dialog ──
    Kirigami.Dialog {
        id: colorDialog
        title: i18n("Pick background color")
        preferredWidth: Kirigami.Units.gridUnit * 18
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

        GridLayout {
            columns: 8
            columnSpacing: 4; rowSpacing: 4
            property var colors: [
                "#ffffff","#f0f0f0","#cccccc","#999999","#666666","#333333","#000000","#1a1a2e",
                "#ff6b6b","#ee5a24","#f0932b","#f9ca24","#6ab04c","#22a6b3","#4834d4","#be2edd",
                "#ff9ff3","#f368e0","#feca57","#ffdd59","#48dbfb","#0abde3","#54a0ff","#2e86de"
            ]
            Repeater {
                model: parent.colors
                Rectangle {
                    width: 32; height: 32; radius: 4
                    border.width: 1; border.color: Kirigami.Theme.textColor
                    color: modelData
                    Controls.Button {
                        anchors.fill: parent; opacity: 0
                        onClicked: {
                            processor.autoColor = false;
                            processor.backgroundColor = modelData;
                        }
                    }
                }
            }
        }
    }

    // ── Processor ──
    WallpaperProcessor {
        id: processor

        onStatusMessageChanged: {
            if (processor.statusMessage.length > 0) {
                statusMessage.type = Kirigami.MessageType.Information;
                statusMessage.text = processor.statusMessage;
                statusMessage.visible = true;
                statusMessageTimer.restart();
            }
        }
        onErrorOccurred: function (msg) {
            statusMessage.type = Kirigami.MessageType.Error;
            statusMessage.text = msg;
            statusMessage.visible = true;
            statusMessageTimer.stop();  // persist errors
        }
        onProcessingFinished: {
            if (processor.outputPath.length > 0) {
                crossfadePreview("file://" + processor.outputPath);
            }
            statusMessage.type = Kirigami.MessageType.Positive;
            statusMessage.text = i18n("Done");
            statusMessage.visible = true;
            statusMessageTimer.restart();
        }
    }

    // Status auto-hide timer
    Timer {
        id: statusMessageTimer
        interval: 4000
        onTriggered: statusMessage.visible = false
    }

    // ── Live preview update on tweak changes (debounced) ──

    Timer {
        id: previewDebounce
        interval: 700
        repeat: false
        onTriggered: refreshPreviews()
    }

    function refreshPreviews() {
        var list = dropArea.fileList;
        if (list.length === 0) return;
        var cacheBust = "?t=" + Date.now();
        var entries = [];
        for (var i = 0; i < list.length; ++i) {
            if (list[i].path) {
                var url = processor.generatePreview(list[i].path);
                if (url.length > 0)
                    url += cacheBust;
                entries.push({path: list[i].path, previewUrl: url});
            }
        }
        dropArea.fileList = entries;
        previewList.model = dropArea.fileList;
        if (dropArea.fileList.length > 0)
            crossfadePreview(dropArea.fileList[0].previewUrl);
    }

    function crossfadePreview(newUrl) {
        // Skip if no animation possible (first load)
        if (!previewA.source.toString() || previewA.source.toString() === "") {
            previewA.source = newUrl;
            return;
        }
        // Skip if same source (cache-bust only, no visual change)
        if (previewA.source.toString() === newUrl && previewB.source.toString() !== newUrl) {
            return;
        }
        // Cancel any running crossfade
        fadeAnim.stop();
        previewA.opacity = 1.0;
        previewB.opacity = 0.0;
        previewB.source = "";
        // Start crossfade to new image
        previewB.source = newUrl;
        fadeAnim.start();
    }

    Connections {
        target: processor
        function onBlurRadiusChanged() { previewDebounce.restart(); }
        function onSaturationFactorChanged() { previewDebounce.restart(); }
        function onBgGradientStyleChanged() { previewDebounce.restart(); }
        function onBgGradientPresetChanged() { previewDebounce.restart(); }
        function onGradientAngleChanged() { previewDebounce.restart(); }
        function onBgZoomChanged() { previewDebounce.restart(); }
        function onBgBlurAngleChanged() { previewDebounce.restart(); }
        function onBlurModeChanged() { previewDebounce.restart(); }
        function onAutoColorChanged() { previewDebounce.restart(); }
        function onBackgroundColorChanged() { previewDebounce.restart(); }
        function onAspectModeChanged() { previewDebounce.restart(); }
        function onAutoMoodChanged() { previewDebounce.restart(); }
        function onTargetWidthChanged() { previewDebounce.restart(); }
        function onTargetHeightChanged() { previewDebounce.restart(); }
        function onVignetteStrengthChanged() { previewDebounce.restart(); }
        function onGrainStrengthChanged() { previewDebounce.restart(); }
        function onCaStrengthChanged() { previewDebounce.restart(); }
        function onPhotoFrameChanged() { previewDebounce.restart(); }
        function onPhotoFrameWidthChanged() { previewDebounce.restart(); }
    }

    Connections {
        target: processor
        function onTargetWidthChanged() { widthInput.text = processor.targetWidth; }
        function onTargetHeightChanged() { heightInput.text = processor.targetHeight; }
    }
}
