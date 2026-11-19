import QtQuick
import Quickshell
import qs.Commons
import qs.Commons as Commons
import qs.Ui

// Spruchband in the bar: a little plane. Click it for the panel, right-click
// to send a banner across the screens now, middle-click to pause or resume.
// The panel says what the service is doing and holds the quick controls;
// figures, styles, sayings and timing live in the settings window.
Panel {
  id: root

  moduleName: "tkenner.spruchband"
  ipcTarget: "tkenner.spruchband"

  // --------------------------------------------------------------- service
  //
  // The shared service owns the program; a bar exists per monitor, and all
  // of them talk through it.
  property var service: null

  function bindService() {
    if (service) return
    var host = bar && bar.shell ? bar.shell : null
    if (!host || typeof host.serviceFor !== "function") return
    service = host.serviceFor("tkenner.spruchband")
  }

  onBarChanged: bindService()
  onOpenedChanged: if (opened && service) service.refresh()

  Timer {
    interval: 200
    running: root.service === null
    repeat: true
    onTriggered: root.bindService()
  }

  readonly property bool running: service ? service.running : false
  readonly property bool enabled: service ? service.enabled : false
  readonly property bool paused: service ? service.paused : false
  readonly property bool active: running && enabled && !paused

  // ---------------------------------------------------------------- words
  //
  // The panel's own words, in the language Omarchy runs in; the status line
  // comes translated from the program itself.
  Commons.Translations {
    id: strings
    directory: Qt.resolvedUrl("i18n")
  }

  // ------------------------------------------------------------------ bar

  implicitWidth: button.implicitWidth
  implicitHeight: button.implicitHeight

  BarIconButton {
    id: button
    anchors.fill: parent
    bar: root.bar
    // nf-md-airplane
    text: "\udb80\udc1d"
    opacity: root.active ? 1 : 0.45
    tooltipText: ""
    onPressed: function(b) {
      if (!root.service) return
      if (b === Qt.RightButton) root.service.now()
      else if (b === Qt.MiddleButton) root.service.togglePause()
      else root.toggle()
    }
  }

  // ---------------------------------------------------------------- panel

  KeyboardPanel {
    id: panel
    anchorItem: button
    owner: root
    bar: root.bar
    open: root.opened
    focusTarget: panel.loadedContent
    contentWidth: panel.fittedContentWidth(Style.space(380))
    contentHeight: panel.fittedContentHeight(panel.loadedContentHeight)

    content: PanelKeyCatcher {
      anchors.fill: parent
      implicitHeight: column.implicitHeight
      onCloseRequested: root.close()
      onTabRequested: function(direction) { root.switchPanel(direction) }
      onActivateRequested: if (root.service) root.service.now()

      Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Style.space(14)

        // ---------- Hero: plane · name · what it is doing ----------
        Item {
          width: parent.width
          implicitHeight: Math.max(heroIcon.implicitHeight, heroLabels.implicitHeight)

          Text {
            id: heroIcon
            textFormat: Text.PlainText
            text: "\udb80\udc1d"
            color: root.bar.foreground
            opacity: root.active ? 1 : 0.5
            font.family: root.bar.fontFamily
            font.pixelSize: Style.font.display
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
          }

          Column {
            id: heroLabels
            anchors.left: heroIcon.right
            anchors.leftMargin: Style.space(14)
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: Style.space(2)

            Text {
              text: "Spruchband"
              textFormat: Text.PlainText
              color: root.bar.foreground
              font.family: root.bar.fontFamily
              font.pixelSize: Style.font.title
              font.bold: true
              elide: Text.ElideRight
              width: parent.width
            }

            Text {
              textFormat: Text.PlainText
              text: root.service && root.service.statusText !== "" ? root.service.statusText : strings.t("panel.starting")
              color: Qt.darker(root.bar.foreground, 1.4)
              font.family: root.bar.fontFamily
              font.pixelSize: Style.font.caption
              wrapMode: Text.WordWrap
              width: parent.width
            }
          }
        }

        PanelSeparator {
          foreground: root.bar.foreground
        }

        Toggle {
          width: parent.width
          label: strings.t("panel.show")
          description: strings.t("panel.showHint")
          checked: root.enabled
          foreground: root.bar.foreground
          fontFamily: root.bar.fontFamily
          onClicked: if (root.service) root.service.setEnabled(!root.enabled)
        }

        Row {
          id: actions
          width: parent.width
          spacing: Style.space(6)
          readonly property real cellWidth: (width - spacing * 2) / 3

          Button {
            width: actions.cellWidth
            iconText: "\udb80\udc1d"
            text: strings.t("panel.now")
            fontSize: Style.font.bodySmall
            foreground: root.bar.foreground
            fontFamily: root.bar.fontFamily
            bordered: true
            enabled: root.running && root.enabled
            opacity: enabled ? 1 : 0.5
            onClicked: root.service.now()
          }

          Button {
            width: actions.cellWidth
            // nf-md-pause / nf-md-play
            iconText: root.paused ? "\udb81\udc0a" : "\udb80\udfe4"
            text: root.paused ? strings.t("panel.resume") : strings.t("panel.pause")
            fontSize: Style.font.bodySmall
            foreground: root.bar.foreground
            fontFamily: root.bar.fontFamily
            bordered: true
            active: root.paused
            enabled: root.running && root.enabled
            opacity: enabled ? 1 : 0.5
            onClicked: root.service.togglePause()
          }

          Button {
            width: actions.cellWidth
            // nf-md-cog
            iconText: "\udb81\udc93"
            text: strings.t("panel.settings")
            fontSize: Style.font.bodySmall
            foreground: root.bar.foreground
            fontFamily: root.bar.fontFamily
            bordered: true
            onClicked: {
              root.close()
              if (root.service) root.service.openSettings()
            }
          }
        }
      }
    }
  }
}
