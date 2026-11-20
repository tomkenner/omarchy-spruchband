// What the desktop is doing right now: the Linux counterpart of the Android
// service's screen, keyguard, Do Not Disturb and full-screen checks. Asks
// Hyprland over its IPC socket and reads the Omarchy shell's DND state.
#pragma once

#include <QHash>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QTimer>

struct MonitorInfo {
    QString name;
    int reservedTop = 0;
    int reservedBottom = 0;
    bool focused = false;
    bool dpms = true;
};

struct DeskState {
    bool hyprland = false; // Hyprland answered; without it only DND is known
    bool locked = false;
    bool screensOff = false;
    bool fullscreen = false;
    bool dnd = false;
    QHash<QString, MonitorInfo> monitors;
    QString focused;
};

namespace Desk {
QString hyprSocket(const QString &name);
QByteArray hyprRequest(const QByteArray &command, int timeoutMs = 500);
// Parses Hyprland's j/monitors and j/clients answers (tests feed them in).
DeskState parse(const QByteArray &monitors, const QByteArray &clients);
DeskState query();
// Do Not Disturb of the Omarchy shell (quiet.json), or mako's mode.
bool dnd(const QString &quietFile = {});
}

// Hyprland's event socket: full-screen changes and monitors coming and going.
class HyprEvents : public QObject {
    Q_OBJECT
public:
    explicit HyprEvents(QObject *parent = nullptr);

signals:
    void received(const QString &name, const QString &data);

private:
    void connectSocket();
    QLocalSocket m_socket;
    QTimer m_retry;
    QByteArray m_buffer;
};
