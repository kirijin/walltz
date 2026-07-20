import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
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
                Layout.preferredHeight: Math.min(
                    width / _ar,
                    root.height * 0.4
                )

                readonly property color _canvasColor: Kirigami.Theme.colorScheme === Kirigami.Theme.Dark
                    ? Qt.darker(Kirigami.Theme.backgroundColor, 2.5)
                    : Qt.darker(Kirigami.Theme.backgroundColor, 3.0)
                Rectangle {
                    id: dropZone
                    anchors.centerIn: parent

                    readonly property double _scale: Math.min(
                        parent.width / previewBox._ar,
                        parent.height
                    )
                    width: previewBox._ar * _scale
                    height: _scale

                    radius: Kirigami.Units.smallSpacing
                    color: dropArea.containsDrag ? Kirigami.Theme.highlightColor
                                                 : _canvasColor
                    border.color: Qt.darker(_canvasColor, 1.2)
                    border.width: 1
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
                                    imagePreview.source = fileList[0].previewUrl;
                                drop.accept();
                            }
                        }
                        onEntered: function (drag) { if (drag.hasUrls) drag.accept(); }
                    }

                    Image {
                        id: imagePreview
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        visible: dropArea.fileCount > 0
                        smooth: true
                        clip: true
                        asynchronous: true
                    }

                    Kirigami.LoadingPlaceholder {
                        anchors.centerIn: parent
                        visible: dropArea.fileCount === 0 && !dropArea.containsDrag
                        text: i18n("Drop image(s) here")
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
                Controls.Label { text: "\u00D7" }
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
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 0
                        onClicked: processor.aspectMode = 0
                    }
                    Controls.Button {
                        text: "1:1"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 1
                        onClicked: processor.aspectMode = 1
                    }
                    Controls.Button {
                        text: "4:3"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 2
                        onClicked: processor.aspectMode = 2
                    }
                    Controls.Button {
                        text: "16:10"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 4
                        onClicked: processor.aspectMode = 4
                    }

                    // Separator
                    Rectangle {
                        width: 1; height: Kirigami.Units.gridUnit * 1.5
                        color: Kirigami.Theme.disabledTextColor
                        Layout.leftMargin: Kirigami.Units.smallSpacing
                        Layout.rightMargin: Kirigami.Units.smallSpacing
                    }

                    // Wide zone
                    Controls.Button {
                        text: "16:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 3
                        onClicked: processor.aspectMode = 3
                    }
                    Controls.Button {
                        text: "21:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 5
                        onClicked: processor.aspectMode = 5
                    }
                    Controls.Button {
                        text: "32:9"
                        flat: true
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 4
                        Controls.ButtonGroup.group: ratioGroup
                        checked: processor.aspectMode === 6
                        onClicked: processor.aspectMode = 6
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // === Background mode toggle ===
            Controls.ButtonGroup { id: modeGroup }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Item { Layout.fillWidth: true }

                Controls.Button {
                    text: i18n("Blur")
                    checkable: true
                    implicitWidth: Kirigami.Units.gridUnit * 7
                    checked: processor.blurMode
                    onClicked: processor.blurMode = true
                    Controls.ButtonGroup.group: modeGroup
                }
                Controls.Button {
                    text: i18n("Colour")
                    checkable: true
                    implicitWidth: Kirigami.Units.gridUnit * 7
                    checked: !processor.blurMode
                    onClicked: processor.blurMode = false
                    Controls.ButtonGroup.group: modeGroup
                }

                Item { Layout.fillWidth: true }
            }

            // ── Blur tweaks (visible when Blur selected) ──
            ColumnLayout {
                visible: processor.blurMode
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                // Blur radius
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "blur"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
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
                    Controls.SpinBox {
                        id: blurSpinBox
                        Layout.preferredWidth: 80
                        from: 0; to: 120
                        value: processor.blurRadius
                        editable: true
                        textFromValue: function(v) { return v === 0 ? i18n("Auto") : String(v); }
                        valueFromText: function(t) {
                            var v = parseInt(t);
                            return isNaN(v) ? 0 : Math.max(0, Math.min(120, v));
                        }
                        onValueModified: processor.blurRadius = value
                    }

                    Item { Layout.fillWidth: true }
                }

                // Saturation
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "color-management"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
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
                    Controls.SpinBox {
                        id: satSpinBox
                        Layout.preferredWidth: 80
                        from: 0; to: 30
                        value: processor.saturationFactor * 10
                        editable: true
                        textFromValue: function(v) { return (v / 10.0).toFixed(1); }
                        valueFromText: function(t) {
                            var v = parseFloat(t);
                            return isNaN(v) ? 10 : Math.max(0, Math.min(30, Math.round(v * 10)));
                        }
                        onValueModified: processor.saturationFactor = value / 10.0
                    }

                    Item { Layout.fillWidth: true }
                }

                // Background zoom
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "zoom-original"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Controls.Slider {
                        id: zoomSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 5; to: 30; stepSize: 1
                        value: processor.bgZoom * 10
                        Controls.ToolTip.text: i18n("%1×").arg(processor.bgZoom.toFixed(1))
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: 400
                        onMoved: processor.bgZoom = value / 10.0
                    }
                    Controls.SpinBox {
                        id: zoomSpinBox
                        Layout.preferredWidth: 80
                        from: 5; to: 30
                        value: processor.bgZoom * 10
                        editable: true
                        textFromValue: function(v) { return (v / 10.0).toFixed(1); }
                        valueFromText: function(t) {
                            var v = parseFloat(t);
                            return isNaN(v) ? 10 : Math.max(5, Math.min(30, Math.round(v * 10)));
                        }
                        onValueModified: processor.bgZoom = value / 10.0
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // ── Post-processing effects (visible on all background styles) ──
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                // Vignette
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "contrast"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Controls.Switch {
                        checked: processor.vignetteStrength > 0
                        onToggled: {
                            processor.vignetteStrength = checked ? 0.5 : 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 1.0; stepSize: 0.05
                        value: processor.vignetteStrength
                        enabled: processor.vignetteStrength > 0
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

                // Grain
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "noise"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Controls.Switch {
                        checked: processor.grainStrength > 0
                        onToggled: {
                            processor.grainStrength = checked ? 0.5 : 0.0
                            previewDebounce.restart()
                        }
                    }
                    Controls.Slider {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 1.0; stepSize: 0.05
                        value: processor.grainStrength
                        enabled: processor.grainStrength > 0
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
            }

            // ── Color/gradient controls (visible when Color selected) ──
            ColumnLayout {
                visible: !processor.blurMode
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                // Style selector — toggle buttons
                Controls.ButtonGroup { id: fillGroup }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Controls.Button {
                        text: i18n("Colour")
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        checked: processor.bgGradientStyle === 0
                        onClicked: processor.bgGradientStyle = 0
                        Controls.ButtonGroup.group: fillGroup
                    }
                    Controls.Button {
                        text: i18n("Gradient")
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        checked: processor.bgGradientStyle === 1
                        onClicked: processor.bgGradientStyle = 1
                        Controls.ButtonGroup.group: fillGroup
                    }
                    Controls.Button {
                        text: i18n("Auto")
                        checkable: true
                        implicitWidth: Kirigami.Units.gridUnit * 5
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
                                imagePreview.source = url + "?t=" + Date.now()
                        }
                    }
                }

                RowLayout {
                    visible: processor.bgGradientStyle === 2
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: 6
                        delegate: Controls.Button {
                            text: processor.moodName(index)
                            checkable: true
                            checked: !processor.useV2 && processor.autoMood === index
                            onClicked: {
                                processor.autoMood = index
                                processor.useV2 = false
                                moodPreviewTimer.restart()
                            }
                            Layout.fillWidth: true
                            Controls.ButtonGroup.group: moodGroup
                        }
                    }
                }

                // Second row: V2 suggestions (3D RGB histogram)
                RowLayout {
                    visible: processor.bgGradientStyle === 2
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: 6
                        delegate: Controls.Button {
                            text: processor.moodNameV2(index)
                            checkable: true
                            checked: processor.useV2 && processor.autoMood === index
                            onClicked: {
                                processor.autoMood = index
                                processor.useV2 = true
                                moodPreviewTimer.restart()
                            }
                            Layout.fillWidth: true
                        }
                    }
                }

                // Solid color chooser — inline palette
                RowLayout {
                    visible: processor.bgGradientStyle === 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    // Auto square — spaced apart
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

                    // Visual spacer after auto
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

                    // More button (opens full dialog)
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

                // Gradient preset picker
                RowLayout {
                    visible: processor.bgGradientStyle === 1
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

                // Gradient angle (only for gradient modes)
                RowLayout {
                    visible: processor.bgGradientStyle > 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "transform-rotate"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Controls.Slider {
                        id: angleSlider
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                        from: 0; to: 360; stepSize: 1
                        value: processor.gradientAngle
                        Controls.ToolTip.text: i18n("%1°").arg(processor.gradientAngle)
                        Controls.ToolTip.visible: hovered
                        onMoved: processor.gradientAngle = value
                    }
                    Controls.SpinBox {
                        Layout.preferredWidth: 80
                        from: 0; to: 360
                        value: processor.gradientAngle
                        editable: true
                        onValueModified: processor.gradientAngle = value
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // === File list (batch) ===
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
                imagePreview.source = "file://" + processor.outputPath;
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
        interval: 300
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
            imagePreview.source = dropArea.fileList[0].previewUrl;
    }

    Connections {
        target: processor
        function onBlurRadiusChanged() { previewDebounce.restart(); }
        function onSaturationFactorChanged() { previewDebounce.restart(); }
        function onBgGradientStyleChanged() { previewDebounce.restart(); }
        function onBgGradientPresetChanged() { previewDebounce.restart(); }
        function onGradientAngleChanged() { previewDebounce.restart(); }
        function onBgZoomChanged() { previewDebounce.restart(); }
        function onBlurModeChanged() { previewDebounce.restart(); }
        function onAutoColorChanged() { previewDebounce.restart(); }
        function onBackgroundColorChanged() { previewDebounce.restart(); }
        function onTargetWidthChanged() { previewDebounce.restart(); }
        function onTargetHeightChanged() { previewDebounce.restart(); }
        function onVignetteStrengthChanged() { previewDebounce.restart(); }
        function onGrainStrengthChanged() { previewDebounce.restart(); }
    }

    Connections {
        target: processor
        function onTargetWidthChanged() { widthInput.text = processor.targetWidth; }
        function onTargetHeightChanged() { heightInput.text = processor.targetHeight; }
    }
}
