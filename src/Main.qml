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
            // C++ detectScreenSize has a QTimer retry loop (10×200ms)
            // that waits for the Wayland wl_output protocol events to arrive.
            processor.detectScreenSize();

            // QML-side fallback: Screen.width/height are invalid at
            // Component.onCompleted (window not yet displayed). Defer
            // until the window is actually on a screen.
            Qt.callLater(function() {
                // Try Screen attached — valid once the page item is displayed
                var w = Screen.width;
                var h = Screen.height;
                if (w > 0 && h > 0) {
                    processor.detectFromQML(w, h, Screen.devicePixelRatio);
                } else {
                    // Last resort: try again after a short delay
                    timerDetect.start();
                }
            });
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
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
                    ToolTip.text: i18n("Reset to screen resolution (%1\u00D7%2)",
                                      processor.screenWidth, processor.screenHeight)
                    ToolTip.visible: resetResBtn.hovered
                    onClicked: processor.detectScreenSize()
                }

                Controls.Label {
                    id: detectedRes
                    text: processor.screenWidth + "\u00D7" + processor.screenHeight
                    color: Kirigami.Theme.disabledTextColor
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                }
            }

            // === Background mode row ===
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

                Item { Layout.fillWidth: true }

                RowLayout {
                    visible: !processor.blurMode
                    spacing: Kirigami.Units.smallSpacing

                    Controls.CheckBox {
                        text: i18n("Auto")
                        checked: processor.autoColor
                        onClicked: processor.autoColor = checked
                    }

                    Rectangle {
                        id: colorSwatch
                        width: 28; height: 28
                        radius: 4
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

                Item { width: Kirigami.Units.smallSpacing }
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
                Layout.preferredHeight: Math.min(count * 28, 100)
                model: []
                delegate: Controls.Label {
                    text: model.modelData ? model.modelData.path.toString().split("/").pop() : ""
                    elide: Text.ElideMiddle
                    leftPadding: Kirigami.Units.smallSpacing
                }
            }

            // === Progress bar ===
            Controls.ProgressBar {
                id: progressBar
                Layout.fillWidth: true
                visible: processor.busy
                // Single image: indeterminate (instant 0→1 jump).
                // Batch: determinate with queue progress.
                indeterminate: processor.queueSize <= 1
                from: 0; to: processor.queueSize || 1
                value: processor.queueProgress
            }

            // === Status ===
            Controls.Label {
                id: statusLabel
                Layout.fillWidth: true
                visible: text.length > 0
                wrapMode: Text.Wrap
                color: Kirigami.Theme.positiveTextColor
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

        onStatusMessageChanged: { statusLabel.text = processor.statusMessage; }
        onErrorOccurred: function (msg) {
            statusLabel.text = msg;
            statusLabel.color = Kirigami.Theme.negativeTextColor;
        }
        onProcessingFinished: {
            // Show the processed output in the preview
            if (processor.outputPath.length > 0)
                imagePreview.source = "file://" + processor.outputPath;
        }
        onScreenWidthChanged: {
            detectedRes.text = processor.screenWidth + "\u00D7" + processor.screenHeight;
        }
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
                processor.detectFromQML(w, h, Screen.devicePixelRatio);
                timerDetect.stop();
            }
        }
    }

    Connections {
        target: processor
        function onTargetWidthChanged() { widthInput.text = processor.targetWidth; }
        function onTargetHeightChanged() { heightInput.text = processor.targetHeight; }
    }
}
