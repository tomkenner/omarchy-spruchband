#include "desk.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>

namespace Desk {

QString hyprSocket(const QString &name) {
    const QString sig = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    if (sig.isEmpty()) return {};
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const QString path = runtime + "/hypr/" + sig + "/" + name;
    if (!runtime.isEmpty() && QFile::exists(path)) return path;
    return "/tmp/hypr/" + sig + "/" + name;
}

QByteArray hyprRequest(const QByteArray &command, int timeoutMs) {
    const QString path = hyprSocket(".socket.sock");
    if (path.isEmpty()) return {};
    QLocalSocket s;
    s.connectToServer(path);
    if (!s.waitForConnected(timeoutMs)) return {};
    s.write(command);
    s.waitForBytesWritten(timeoutMs);
    QByteArray out;
    // Hyprland answers and closes the connection.
    while (s.state() == QLocalSocket::ConnectedState && s.waitForReadyRead(timeoutMs)) out += s.readAll();
    out += s.readAll();
    return out;
}

DeskState parse(const QByteArray &monitors, const QByteArray &clients) {
    DeskState d;
    const QJsonArray mons = QJsonDocument::fromJson(monitors).array();
    if (mons.isEmpty()) return d;
    d.hyprland = true;
    QSet<int> visible;
    bool anyOn = false;
    for (const QJsonValue &v : mons) {
        const QJsonObject m = v.toObject();
        MonitorInfo info;
        info.name = m.value("name").toString();
        const QJsonArray reserved = m.value("reserved").toArray();
        info.reservedTop = reserved.at(1).toInt();
        info.reservedBottom = reserved.at(3).toInt();
        info.focused = m.value("focused").toBool();
        info.dpms = m.value("dpmsStatus").toBool(true);
        anyOn = anyOn || info.dpms;
        if (info.focused) d.focused = info.name;
        // Hyprland has no lock flag, but an active session lock is one of the
        // reasons a monitor cannot go solitary (see omarchy-hyprland-session-locked).
        if (m.value("solitaryBlockedBy").toArray().contains(QJsonValue("LOCK"))) d.locked = true;
        visible.insert(m.value("activeWorkspace").toObject().value("id").toInt());
        const int special = m.value("specialWorkspace").toObject().value("id").toInt();
        if (special != 0) visible.insert(special);
        d.monitors.insert(info.name, info);
    }
    d.screensOff = !anyOn;
    // Real full screen (videos, games) hides the bar, like Android's immersive
    // mode; a maximised window does not count.
    for (const QJsonValue &v : QJsonDocument::fromJson(clients).array()) {
        const QJsonObject c = v.toObject();
        if ((c.value("fullscreen").toInt() & 2) && c.value("mapped").toBool(true) &&
            visible.contains(c.value("workspace").toObject().value("id").toInt()))
            d.fullscreen = true;
    }
    return d;
}

DeskState query() {
    DeskState d = parse(hyprRequest("j/monitors"), hyprRequest("j/clients"));
    d.dnd = dnd();
    return d;
}

bool dnd(const QString &quietFile) {
    const QString path = !quietFile.isEmpty()
                           ? quietFile
                           : QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
                                 "/omarchy/notifications/quiet.json";
    QFile f(path);
    if (f.open(QIODevice::ReadOnly))
        return QJsonDocument::fromJson(f.readAll()).object().value("dnd").toBool();
    if (!quietFile.isEmpty()) return false;
    // Older Omarchy versions ran mako, whose DND is a mode.
    const QString mako = QStandardPaths::findExecutable("makoctl");
    if (mako.isEmpty()) return false;
    QProcess p;
    p.start(mako, {"mode"});
    if (!p.waitForFinished(500)) {
        p.kill();
        return false;
    }
    return p.readAllStandardOutput().contains("do-not-disturb");
}
}

HyprEvents::HyprEvents(QObject *parent) : QObject(parent) {
    m_retry.setSingleShot(true);
    m_retry.setInterval(3000);
    connect(&m_retry, &QTimer::timeout, this, &HyprEvents::connectSocket);
    connect(&m_socket, &QLocalSocket::disconnected, &m_retry, qOverload<>(&QTimer::start));
    connect(&m_socket, &QLocalSocket::errorOccurred, this, [this] {
        if (m_socket.state() != QLocalSocket::ConnectedState) m_retry.start();
    });
    connect(&m_socket, &QLocalSocket::readyRead, this, [this] {
        m_buffer += m_socket.readAll();
        qsizetype nl;
        while ((nl = m_buffer.indexOf('\n')) >= 0) {
            const QString line = QString::fromUtf8(m_buffer.left(nl));
            m_buffer.remove(0, nl + 1);
            const qsizetype sep = line.indexOf(">>");
            if (sep > 0) emit received(line.left(sep), line.mid(sep + 2));
        }
    });
    connectSocket();
}

void HyprEvents::connectSocket() {
    const QString path = Desk::hyprSocket(".socket2.sock");
    if (path.isEmpty()) return;
    m_buffer.clear();
    m_socket.abort();
    m_socket.connectToServer(path);
}
