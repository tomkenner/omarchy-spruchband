import QtQuick
import Quickshell
import Quickshell.Io

// Spruchband's service, mounted once for the shell.
//
// The banners themselves are drawn by bin/spruchband, a native program with
// one transparent, click-through layer-shell strip per screen, so a figure
// can fly from the left edge of one monitor to the right edge of the other.
// Keeping the drawing out of the shell process means a heavy frame or a crash
// never touches the bar. This service owns that program: it starts with the
// shell, comes back if it dies, and goes when the plugin is disabled.
//
// The two talk over the program's stdin and stdout only: a command per line
// in, a line of JSON out whenever its state changes. (No local socket here:
// the program hangs up after every socket answer, which Quickshell's Socket
// does not survive.)
//
// Bars are built per monitor; the widgets all read their state from here and
// send their commands through here, so every screen agrees.
Item {
  id: root
  width: 0
  height: 0
  visible: false

  property var shell: null
  property var manifest: null
  property var pluginRegistry: null
  property var barWidgetRegistry: null
  property string omarchyPath: ""

  readonly property string binary: Qt.resolvedUrl("bin/spruchband").toString().replace(/^file:\/\//, "")

  // What the program last reported.
  property bool running: false
  property bool enabled: true
  property bool paused: false
  property bool flying: false
  property string statusText: ""

  function send(command) {
    if (program.running) program.write(command + "\n")
  }
  function now() { send("now") }
  function pause(minutes) { send("pause " + Math.max(1, Math.round(minutes || 60))) }
  function resume() { send("resume") }
  function togglePause() { if (paused) resume(); else pause(60) }
  function setEnabled(on) { send(on ? "enable" : "disable") }
  function refresh() { send("status") }
  function openSettings() { Quickshell.execDetached([root.binary]) }

  // Only lines that look like the program's status are taken; anything else
  // (an error answer, stray output) is ignored.
  function absorb(line) {
    var data
    try {
      data = JSON.parse(line)
    } catch (e) {
      return
    }
    if (!data || typeof data !== "object" || data.running !== true) return
    running = true
    enabled = data.enabled === true
    paused = data.paused === true
    flying = data.flying === true
    statusText = String(data.text || "").slice(0, 300)
  }

  property int failures: 0

  Process {
    id: program
    command: [root.binary, "daemon", "--stdio"]
    running: true
    stdinEnabled: true
    stdout: SplitParser {
      onRead: function(line) { root.absorb(line) }
    }
    onExited: function(exitCode) {
      root.running = false
      root.flying = false
      // A crash loop backs off instead of hammering the machine.
      root.failures = exitCode === 0 ? 0 : root.failures + 1
      restart.interval = Math.min(60000, 2000 * Math.pow(2, Math.min(root.failures, 5)))
      restart.restart()
    }
  }

  Timer {
    id: restart
    onTriggered: if (!program.running) program.running = true
  }
}
