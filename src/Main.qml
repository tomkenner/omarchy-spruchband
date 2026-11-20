import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Spruchband

ApplicationWindow {
    id: win
    width: 820
    height: 920
    minimumWidth: 560
    minimumHeight: 480
    visible: true
    title: "Spruchband"
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    readonly property color muted: "#9a9aa3"
    readonly property bool typing: editor.opened
    readonly property var hours: Array.from({ length: 24 }, (_, i) => (i < 10 ? "0" : "") + i + ":00")

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: help.opened ? help.close() : help.open() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.showNow() }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.paused ? backend.resume() : backend.pause(60) }
    Shortcut { sequence: "N"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: editor.openNew() }

    Connections {
        target: backend
        function onMessage(text) { toast.show(text) }
    }

    // Labels show plain text: what comes from files or the background service
    // (collection names, messages, status) is never read as markup, so it can
    // neither style the window nor load an image.
    component Hint: Label {
        textFormat: Text.PlainText
        color: win.muted
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ScrollView {
            id: scroller
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: Math.min(scroller.availableWidth - 32, 760)
                x: (scroller.availableWidth - width) / 2
                spacing: 12

                Item { Layout.preferredHeight: 8 }

                Label {
                    text: "Spruchband"
                    font.pixelSize: 30
                    font.bold: true
                }
                Hint {
                    text: qsTr("Every few minutes a figure crosses your screens pulling a saying behind it.")
                    font.pixelSize: 15
                }

                // ---------- Preview ----------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    Layout.topMargin: 4
                    radius: theme.radius
                    clip: true
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#241A4A" }
                        GradientStop { position: 1; color: "#0D0B1F" }
                    }
                    StagePreview {
                        id: preview
                        anchors.fill: parent
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: preview.next("")
                    }
                }
                Hint { text: qsTr("Preview with your settings. Click it to see the next one.") }

                // ---------- Status ----------
                SectionCard {
                    title: qsTr("Status")
                    hint: qsTr("Nothing is shown while the screen is locked or off, or while a video or game runs full screen. A banner that falls due meanwhile follows a few seconds later.")

                    Switch {
                        text: qsTr("Show banners")
                        checked: backend.enabled
                        onToggled: backend.enabled = checked
                    }
                    Label {
                        text: backend.statusText
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.bottomMargin: 4
                    }
                    RowLayout {
                        spacing: 8
                        Button {
                            text: qsTr("Show one now")
                            highlighted: true
                            enabled: backend.enabled && backend.running
                            onClicked: backend.showNow()
                        }
                        Button {
                            text: backend.paused ? qsTr("Resume") : qsTr("Pause 1 h")
                            enabled: backend.enabled
                            onClicked: backend.paused ? backend.resume() : backend.pause(60)
                        }
                    }
                    Hint {
                        Layout.topMargin: 6
                        text: qsTr("For a key binding: “spruchband now” shows a banner, “spruchband toggle” pauses for an hour or resumes.")
                    }
                }

                // ---------- Figures ----------
                SectionCard {
                    title: qsTr("Figures")
                    hint: qsTr("Tick figures from any category. Each flyby picks one at random, and the banner matches its kind.")

                    Repeater {
                        model: backend.categoryModel
                        delegate: ColumnLayout {
                            id: catBox
                            required property var modelData
                            readonly property var figs: backend.figureModel.filter(f => f.cat === modelData.id)
                            Layout.fillWidth: true
                            spacing: 0

                            CheckBox {
                                text: qsTr(catBox.modelData.name)
                                font.bold: true
                                checked: catBox.figs.every(f => backend.figures.indexOf(f.id) >= 0)
                                onToggled: backend.setCategory(catBox.modelData.id, checked)
                            }
                            Repeater {
                                model: catBox.figs
                                delegate: CheckBox {
                                    required property var modelData
                                    Layout.leftMargin: 28
                                    text: qsTr(modelData.name) + "  ·  " + qsTr(modelData.kind)
                                    checked: backend.figures.indexOf(modelData.id) >= 0
                                    onToggled: backend.setFigure(modelData.id, checked)
                                }
                            }
                        }
                    }
                }

                // ---------- Styles ----------
                SectionCard {
                    title: qsTr("Styles")
                    hint: qsTr("If you tick several styles, each flyby picks one.")

                    Repeater {
                        model: backend.styleModel
                        delegate: ColumnLayout {
                            id: styleBox
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 0
                            CheckBox {
                                text: qsTr(styleBox.modelData.name)
                                checked: backend.styles.indexOf(styleBox.modelData.id) >= 0
                                onToggled: {
                                    backend.setStyle(styleBox.modelData.id, checked)
                                    if (checked) preview.next(styleBox.modelData.id)
                                }
                            }
                            Hint {
                                Layout.leftMargin: 40
                                Layout.topMargin: -10
                                Layout.bottomMargin: 2
                                text: qsTr(styleBox.modelData.hint)
                            }
                        }
                    }
                }

                // ---------- Collections ----------
                SectionCard {
                    title: qsTr("Collections")
                    hint: qsTr("One saying per line. Sayings from all ticked collections are shuffled, and each one comes up once before any repeats.")

                    Repeater {
                        model: backend.collections
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            CheckBox {
                                id: collBox
                                Layout.fillWidth: true
                                text: modelData.name + "  ·  " + qsTr("%n sayings", "", modelData.count)
                                checked: backend.activeCollections.indexOf(modelData.id) >= 0
                                onToggled: backend.setCollectionActive(modelData.id, checked)
                                // The Material check box's label, but as plain text:
                                // the name is whatever the collection file says.
                                contentItem: Text {
                                    leftPadding: collBox.indicator && !collBox.mirrored ? collBox.indicator.width + collBox.spacing : 0
                                    rightPadding: collBox.indicator && collBox.mirrored ? collBox.indicator.width + collBox.spacing : 0
                                    text: collBox.text
                                    textFormat: Text.PlainText
                                    font: collBox.font
                                    color: collBox.enabled ? collBox.Material.foreground : collBox.Material.hintTextColor
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            Button {
                                text: qsTr("Edit")
                                flat: true
                                onClicked: editor.openFor(modelData.id)
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 8
                        Button { text: qsTr("New collection"); onClicked: editor.openNew() }
                        Button { text: qsTr("Import text file"); onClicked: backend.importCollection() }
                        Button { text: qsTr("Open folder"); flat: true; onClicked: backend.openFolder() }
                    }
                }

                // ---------- Timing ----------
                SectionCard {
                    title: qsTr("Timing")

                    Label { text: qsTr("Every %n minutes", "", backend.interval) }
                    Slider {
                        Layout.fillWidth: true
                        from: 1
                        to: 60
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: backend.interval
                        onMoved: backend.interval = Math.round(value)
                    }
                    Switch {
                        text: qsTr("Vary the interval slightly (±30 %)")
                        checked: backend.jitter
                        onToggled: backend.jitter = checked
                    }
                    Switch {
                        text: qsTr("Pause during Do Not Disturb")
                        checked: backend.respectDnd
                        onToggled: backend.respectDnd = checked
                    }
                    Switch {
                        text: qsTr("Quiet hours")
                        checked: backend.quiet
                        onToggled: backend.quiet = checked
                    }
                    RowLayout {
                        enabled: backend.quiet
                        spacing: 10
                        Layout.leftMargin: 8
                        Label { text: qsTr("from") }
                        ComboBox {
                            model: win.hours
                            currentIndex: backend.quietFrom
                            onActivated: index => backend.quietFrom = index
                        }
                        Label { text: qsTr("to") }
                        ComboBox {
                            model: win.hours
                            currentIndex: backend.quietTo
                            onActivated: index => backend.quietTo = index
                        }
                    }
                }

                // ---------- Look ----------
                SectionCard {
                    title: qsTr("Look")

                    Label { text: qsTr("Figure size: %1 px").arg(backend.figureSize) }
                    Slider {
                        Layout.fillWidth: true
                        from: 36; to: 200; stepSize: 1
                        value: backend.figureSize
                        onMoved: backend.figureSize = Math.round(value)
                    }
                    Label { text: qsTr("Banner height: %1 px").arg(backend.bandHeight) }
                    Slider {
                        Layout.fillWidth: true
                        from: 18; to: 80; stepSize: 1
                        value: backend.bandHeight
                        onMoved: backend.bandHeight = Math.round(value)
                    }
                    Label { text: qsTr("Speed: %1 px/s").arg(backend.speed) }
                    Slider {
                        Layout.fillWidth: true
                        from: 40; to: 600; stepSize: 10
                        value: backend.speed
                        onMoved: backend.speed = Math.round(value)
                    }
                    RowLayout {
                        spacing: 10
                        Label { text: qsTr("Direction") }
                        ComboBox {
                            readonly property var values: [0, 1, -1]
                            Layout.preferredWidth: 240
                            model: [qsTr("Random"), qsTr("Left to right"), qsTr("Right to left")]
                            currentIndex: Math.max(0, values.indexOf(backend.direction))
                            onActivated: index => backend.direction = values[index]
                        }
                    }
                    Switch {
                        text: qsTr("Fly across all screens")
                        checked: backend.allScreens
                        onToggled: backend.allScreens = checked
                    }
                    Hint {
                        Layout.leftMargin: 54
                        Layout.topMargin: -8
                        text: qsTr("Otherwise only on the screen with the focused window.")
                    }
                }

                // ---------- About ----------
                SectionCard {
                    title: qsTr("About")
                    Hint {
                        text: qsTr("Version %1. Spruchband collects no data and needs no internet. Your collections stay on this computer. Fonts: Fredoka, Caveat, Tilt Neon, Press Start 2P, Cormorant Garamond, Bangers, IM Fell English (SIL Open Font License), Permanent Marker (Apache License 2.0).").arg(backend.version)
                    }
                }

                Item { Layout.preferredHeight: 16 }
            }
        }
    }

    CollectionEditor { id: editor }
    HelpOverlay { id: help }

    Popup {
        id: toast
        function show(text) {
            toastLabel.text = text
            open()
            toastTimer.restart()
        }
        x: (win.width - width) / 2
        y: win.height - height - 24
        width: Math.min(implicitWidth, win.width - 48)
        padding: 12
        closePolicy: Popup.NoAutoClose
        background: Rectangle {
            color: "#26262c"
            border.color: "#3a3a42"
            radius: Math.min(8, theme.radius)
        }
        contentItem: Label {
            id: toastLabel
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
        Timer {
            id: toastTimer
            interval: 3500
            onTriggered: toast.close()
        }
    }
}
