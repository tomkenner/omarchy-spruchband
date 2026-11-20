// The background service, the Linux counterpart of the Android app's
// BannerService: waits out the interval, then sends one train across every
// screen. The Omarchy plugin's Service.qml starts it with the shell and stops
// it when the plugin is disabled. While the screen is locked or off, during a pause, quiet hours,
// Do Not Disturb or a full-screen app, a banner that falls due is held back
// and follows GRACE after the block ends.
#pragma once

#include "desk.h"
#include "overlay.h"
#include "settings.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
#include <QTimer>
#include <memory>
#include <optional>
#include <vector>

class Daemon : public QObject {
    Q_OBJECT
public:
    explicit Daemon(QObject *parent = nullptr);
    ~Daemon() override;

    // Takes over from an instance that is already running; false if it won't go.
    bool listen();
    // Sends a train now, whatever the schedule says. Returns an error or "".
    QString showNow();
    // Talks to the Omarchy plugin over stdin and stdout: one command per line
    // in, one line of JSON out for every answer and every change of state.
    // The service quits when the shell closes the pipe.
    void attachStdio();

    static constexpr qint64 GRACE = 15'000;  // after a block ends
    static constexpr qint64 RECHECK = 5'000; // while a due banner is blocked

private:
    void schedule();
    void onTick();
    void onFrame();
    void hide();
    void reload();
    QString blockReason(const DeskState &d) const;
    qint64 interval() const;
    QJsonObject handle(const QByteArray &line);
    QJsonObject command(const QByteArray &cmd, const QList<QByteArray> &parts);
    QJsonObject status() const;
    QString fly(const DeskState &d);
    void publish();
    void emitLine(const QJsonObject &o);

    Settings m_set;
    Picker m_picker{m_set};
    QLocalServer m_server;
    HyprEvents m_events;
    QTimer m_tick, m_frame;
    QElapsedTimer m_clock;
    std::optional<Train> m_train;
    std::vector<std::unique_ptr<Overlay>> m_overlays;
    qint64 m_nextAt = 0;
    bool m_blocked = false;
    QString m_reason;
    bool m_stdio = false;
    QByteArray m_stdin;
    QTimer m_pulse;
    int m_lastInterval = 0;
    bool m_lastJitter = false;
};
