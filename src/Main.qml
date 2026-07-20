import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.walltz.processor 1.0

Kirigami.ApplicationWindow {
    id: root

    width: 560
    height: 720
    minimumWidth: 450
    minimumHeight: 620

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
            Qt.callLater(function() {
                var w = Screen.width;
                var h = Screen.height;
                // Use the window's devicePixelRatio (may be fractional on Wayland,
                // unlike Screen.devicePixelRatio which is integer-only).
                var dpr = processor.windowDpr;
                if (w > 0 && h > 0 && processor.screenWidth <= 1920) {
                    processor.detectFromQML(w, h, dpr);
                }
            });
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing
            anchors.margins: Kirigami.Units.largeSpacing

            // === Drop area / preview ===
            Rectangle {
                id: dropZone
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                color: dropArea.containsDrag ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
                border.color: dropArea.fileCount > 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
                border.width: 2
                radius: Kirigami.Units.smallSpacing
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
                                var url = drop.urls[i].toString();
                                if (url.startsWith("file://"))
                                    url = url.substring(7);
                                if (url.length > 0) {
                                    paths.push(url);
                                    var pv = processor.generatePreview(url);
                                    entries.push({path: url, previewUrl: pv});
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

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.LoadingPlaceholder {
                        visible: dropArea.fileCount === 0 && !dropArea.containsDrag
                        text: i18n("Drop image(s) here")
                    }

                    Image {
                        id: imagePreview
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.maximumHeight: 200
                        fillMode: Image.PreserveAspectFit
                        visible: dropArea.fileCount > 0
                        smooth: true
                        clip: true
                    }

                    Controls.Label {
                        visible: dropArea.fileCount > 1
                        text: i18n("+ %1 more files", dropArea.fileCount - 1)
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        color: Kirigami.Theme.disabledTextColor
                    }
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

            // === Resolution row ===
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Label { text: i18n("Resolution:") }

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
                    onClicked: processor.detectScreenSize()
                }

                Controls.Label {
                    id: detectedRes
                    text: processor.screenWidth + "\u00D7" + processor.screenHeight
                    color: Kirigami.Theme.disabledTextColor
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                }
            }

            // === Background mode + blur tweaks ===
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Label { text: i18n("Background:") }

                Controls.RadioButton {
                    text: i18n("Blur"); checked: processor.blurMode
                    onClicked: processor.blurMode = true
                }
                Controls.RadioButton {
                    text: i18n("Color"); checked: !processor.blurMode
                    onClicked: processor.blurMode = false
                }
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

                    Controls.Label {
                        text: i18n("Blur:")
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 4
                    }
                    Controls.Slider {
                        id: blurSlider
                        Layout.fillWidth: true
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
                }

                // Saturation
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        text: i18n("Saturation:")
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 4
                    }
                    Controls.Slider {
                        id: satSlider
                        Layout.fillWidth: true
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
                }
            }

            // ── Color/gradient controls (visible when Color selected) ──
            ColumnLayout {
                visible: !processor.blurMode
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                // Style selector
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label { text: i18n("Fill:") }

                    Controls.ComboBox {
                        id: fillCombo
                        model: [i18n("Solid"), i18n("Gradient"), i18n("Auto")]
                        currentIndex: processor.bgGradientStyle
                        onActivated: processor.bgGradientStyle = currentIndex
                    }
                }

                // Solid color chooser
                RowLayout {
                    visible: processor.bgGradientStyle === 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.CheckBox {
                        text: i18n("Auto")
                        checked: processor.autoColor
                        onClicked: processor.autoColor = checked
                    }

                    Rectangle {
                        id: colorSwatch
                        width: 28; height: 28
                        radius: Kirigami.Units.cornerRadius
                        border.width: 1
                        border.color: Kirigami.Theme.textColor
                        color: processor.backgroundColor
                        enabled: !processor.autoColor

                        Controls.Button {
                            anchors.fill: parent
                            opacity: 0
                            onClicked: colorDialog.open()
                        }
                    }
                }

                // Gradient preset picker
                GridLayout {
                    visible: processor.bgGradientStyle === 1
                    columns: 5
                    columnSpacing: Kirigami.Units.mediumSpacing
                    rowSpacing: Kirigami.Units.mediumSpacing
                    Layout.fillWidth: true

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

                // Gradient angle (only for gradient modes)
                RowLayout {
                    visible: processor.bgGradientStyle > 0
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        text: i18n("Angle:")
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 4
                    }
                    Controls.Slider {
                        id: angleSlider
                        Layout.fillWidth: true
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
                }
            }

            // === Always on top ===
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.CheckBox {
                    id: keepAboveCb
                    text: i18n("Always on top")
                    checked: processor.keepAbove
                    onClicked: processor.setKeepAbove(checked)
                }

                Item { Layout.fillWidth: true }
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
                        onClicked: processor.backgroundColor = modelData
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
        onScreenWidthChanged: {
            detectedRes.text = processor.screenWidth + "\u00D7" + processor.screenHeight;
        }
    }

    // Status auto-hide timer
    Timer {
        id: statusMessageTimer
        interval: 4000
        onTriggered: statusMessage.visible = false
    }

    // QML-side timer fallback for screen detection
    Timer {
        id: timerDetect
        interval: 300
        repeat: true
        running: false
        onTriggered: {
            var w = Screen.width;
            var h = Screen.height;
            if (w > 0 && h > 0) {
                processor.detectFromQML(w, h, processor.windowDpr);
                timerDetect.stop();
            }
        }
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
        var entries = [];
        for (var i = 0; i < list.length; ++i) {
            if (list[i].path) {
                entries.push({path: list[i].path, previewUrl: processor.generatePreview(list[i].path)});
            }
        }
        dropArea.fileList = entries;
        previewList.model = dropArea.fileList;
        if (dropArea.fileList.length > 0)
            imagePreview.source = dropArea.fileList[0].previewUrl;
    }

    Connections {
        target: processor
        onBlurRadiusChanged: previewDebounce.restart()
        onSaturationFactorChanged: previewDebounce.restart()
        onBgGradientStyleChanged: previewDebounce.restart()
        onBgGradientPresetChanged: previewDebounce.restart()
        onGradientAngleChanged: previewDebounce.restart()
        onBlurModeChanged: previewDebounce.restart()
        onAutoColorChanged: previewDebounce.restart()
        onBackgroundColorChanged: previewDebounce.restart()
    }

    Connections {
        target: processor
        function onTargetWidthChanged() { widthInput.text = processor.targetWidth; }
        function onTargetHeightChanged() { heightInput.text = processor.targetHeight; }
    }
}
