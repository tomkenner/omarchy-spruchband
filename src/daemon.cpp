#include "daemon.h"

#include "ipc.h"
#include "status.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QRandomGenerator>
#include <QScreen>
#include <QSocketNotifier>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unistd.h>

static qint64 now() { return QDateTime::currentMSecsSinceEpoch(); }

Daemon::Daemon(QObject *parent) : QObject(parent) {
    m_tick.setSingleShot(true);
    connect(&m_tick, &QTimer::timeout, this, &Daemon::onTick);
    m_frame.setInterval(33);
    connect(&m_frame, &QTimer::timeout, this, &Daemon::onFrame);
    m_lastInterval = m_set.interval();
    m_lastJitter = m_set.jitter();

    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *c = m_server.nextPendingConnection()) {
            connect(c, &QLocalSocket::disconnected, c, &QObject::deleteLater);
            connect(c, &QLocalSocket::readyRead, this, [this, c] {
                if (!c->canReadLine()) return;
                const QJsonObject answer = handle(c->readLine().trimmed());
                c->write(QJsonDocument(answer).toJson(QJsonDocument::Compact) + "\n");
                c->flush();
                c->disconnectFromServer();
                if (answer.value("quit").toBool()) QTimer::singleShot(50, qApp, &QCoreApplication::quit);
            });
        }
    });

    // A window going full screen takes the train off the screen at once, like
    // a video starting on the phone; monitors coming or going end it too.
    connect(&m_events, &HyprEvents::received, this, [this](const QString &name, const QString &data) {
        if (!m_train) return;
        if (name == "fullscreen" && data == "1" && Desk::query().fullscreen) hide();
        if (name.startsWith("monitoradded") || name.startsWith("monitorremoved")) hide();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] { hide(); });

    schedule();
}

Daemon::~Daemon() {
    m_stdio = false;
    hide();
}

void Daemon::attachStdio() {
    m_stdio = true;
    auto *notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this, notifier] {
        char buf[4096];
        const ssize_t got = ::read(STDIN_FILENO, buf, sizeof buf);
        if (got <= 0) {
            // The shell went away (or disabled the plugin): so do we.
            notifier->setEnabled(false);
            hide();
            QCoreApplication::quit();
            return;
        }
        m_stdin.append(buf, got);
        qsizetype nl;
        while ((nl = m_stdin.indexOf('\n')) >= 0) {
            const QByteArray line = m_stdin.left(nl).trimmed();
            m_stdin.remove(0, nl + 1);
            if (line.isEmpty()) continue;
            const QJsonObject answer = handle(line);
            if (!answer.value("ok").toBool()) emitLine(answer);
            if (line == "status" || line == "ping") publish();
            if (answer.value("quit").toBool()) QTimer::singleShot(50, qApp, &QCoreApplication::quit);
        }
    });
    // "Next banner in about 7 minutes" ages; refresh it now and then.
    m_pulse.setInterval(20'000);
    connect(&m_pulse, &QTimer::timeout, this, &Daemon::publish);
    m_pulse.start();
    publish();
}

void Daemon::publish() {
    if (m_stdio) emitLine(status());
}

void Daemon::emitLine(const QJsonObject &o) {
    fputs(QJsonDocument(o).toJson(QJsonDocument::Compact).append('\n').constData(), stdout);
    fflush(stdout);
}

bool Daemon::listen() {
    // The newest instance wins: one left behind by a shell that crashed hands
    // over to the one the plugin just started.
    if (Ipc::send("quit", 300)) {
        for (int i = 0; i < 20 && Ipc::send("ping", 100); i++) QThread::msleep(100);
        if (Ipc::send("ping", 100)) return false;
    }
    const QString path = Ipc::socketPath();
    QLocalServer::removeServer(path);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    return m_server.listen(path);
}

qint64 Daemon::interval() const {
    const qint64 base = m_set.interval() * 60'000LL;
    return m_set.jitter() ? qint64(base * QRandomGenerator::global()->bounded(0.6) + base * 0.7) : base;
}

void Daemon::schedule() {
    m_tick.stop();
    // Switched off: stay running for the plugin, but plan nothing.
    if (!m_set.enabled()) {
        m_nextAt = 0;
        m_blocked = false;
        return;
    }
    const qint64 t = now();
    // No due time yet, or implausibly far away (clock changed): plan afresh.
    if (m_nextAt == 0 || m_nextAt - t > m_set.interval() * 60'000LL * 2) m_nextAt = t + interval();
    m_tick.start(int(std::max<qint64>(m_nextAt - t, 1000)));
}

QString Daemon::blockReason(const DeskState &d) const {
    if (d.locked) return "locked";
    if (d.screensOff) return "screensOff";
    if (m_set.paused()) return "paused";
    if (m_set.inQuietHours()) return "quiet";
    if (m_set.respectDnd() && d.dnd) return "dnd";
    if (d.fullscreen) return "fullscreen";
    return {};
}

void Daemon::onTick() {
    m_set.sync();
    const DeskState d = Desk::query();
    const QString before = m_blocked ? m_reason : QString();
    m_reason = blockReason(d);
    if (m_reason != before) QTimer::singleShot(0, this, &Daemon::publish);
    if (!m_reason.isEmpty()) {
        // Not allowed right now: stay due and look again shortly.
        m_blocked = true;
        m_tick.start(int(RECHECK));
        return;
    }
    if (m_blocked) {
        // The block just ended: give the person a moment before the banner.
        m_blocked = false;
        m_tick.start(int(GRACE));
        return;
    }
    if (now() < m_nextAt) {
        schedule();
        return;
    }
    m_nextAt = now() + interval();
    fly(d);
    schedule();
}

QString Daemon::showNow() {
    m_set.sync();
    const DeskState d = Desk::query();
    if (d.locked || d.screensOff) return "The screen is locked or off.";
    return fly(d);
}

QString Daemon::fly(const DeskState &d) {
    if (m_train) return "A banner is already flying.";
    const Figure *fig = m_picker.figure();
    if (!fig) return "No figures are ticked.";
    const auto text = m_picker.text();
    if (!text) return "The ticked collections have no sayings.";

    QList<QScreen *> screens;
    for (QScreen *s : QGuiApplication::screens()) {
        const auto info = d.monitors.constFind(s->name());
        if (info != d.monitors.constEnd() && !info->dpms) continue;
        if (!s->geometry().isEmpty()) screens << s;
    }
    if (!m_set.allScreens()) {
        QScreen *only = nullptr;
        for (QScreen *s : screens)
            if (s->name() == d.focused) only = s;
        if (!only) only = QGuiApplication::primaryScreen();
        screens = only ? QList<QScreen *>{only} : QList<QScreen *>{};
    }
    if (screens.isEmpty()) return "No screen to show it on.";

    // The world spans every screen side by side, in Hyprland's layout
    // coordinates; a train enters at the left edge of the leftmost screen and
    // leaves at the right edge of the rightmost one (or the other way round).
    int minX = INT_MAX, maxX = INT_MIN;
    for (QScreen *s : screens) {
        minX = std::min(minX, s->geometry().left());
        maxX = std::max(maxX, s->geometry().left() + s->geometry().width());
    }
    Stage stage(1);
    m_train = stage.makeTrain(*fig, *text, m_picker.style(), m_set.size(), m_set.band(), m_set.speed(),
                              m_set.direction(), float(maxX - minX), m_picker.persons());
    const int h = int(std::ceil(m_train->height())) + 1;
    // Fliers keep the same relative height on every screen; walkers and the
    // car stay on each screen's bottom edge.
    const float f = float(QRandomGenerator::global()->generateDouble());
    for (QScreen *s : screens) {
        const QRect g = s->geometry();
        const MonitorInfo info = d.monitors.value(s->name());
        const float sh = g.height(), top = info.reservedTop, bottom = info.reservedBottom;
        float y;
        if (fig->air) {
            const float lo = top + (sh - top) * 0.08f, hi = top + (sh - top) * 0.55f - h;
            y = lo + f * std::max(0.0f, hi - lo);
        } else {
            y = sh - bottom - h;
        }
        auto o = std::make_unique<Overlay>(s, *m_train, m_clock, float(g.left() - minX), int(y), h);
        o->show();
        m_overlays.push_back(std::move(o));
    }
    m_clock.start();
    onFrame();
    m_frame.start();
    publish();
    return {};
}

void Daemon::onFrame() {
    if (!m_train) return;
    const float el = float(m_clock.nsecsElapsed() / 1e9);
    if (el > m_train->duration() + 0.3f) {
        hide();
        return;
    }
    float lo, hi;
    m_train->span(el, &lo, &hi);
    // A strip goes live a little before the train reaches its screen.
    const float margin = 200;
    for (auto &o : m_overlays) o->setLive(hi + margin > o->offX() && lo - margin < o->offX() + o->width());
}

void Daemon::hide() {
    m_frame.stop();
    m_overlays.clear();
    const bool was = bool(m_train);
    m_train.reset();
    if (was) publish();
}

void Daemon::reload() {
    m_set.sync();
    if (!m_set.enabled()) hide();
    if (m_set.interval() != m_lastInterval || m_set.jitter() != m_lastJitter) {
        m_lastInterval = m_set.interval();
        m_lastJitter = m_set.jitter();
        m_nextAt = 0;
    }
    schedule();
}

QJsonObject Daemon::status() const {
    QJsonObject o{
        {"ok", true},
        {"running", true},
        {"enabled", m_set.enabled()},
        {"paused", m_set.paused()},
        {"pausedUntil", double(m_set.pausedUntil())},
        {"nextAt", double(m_nextAt)},
        {"blocked", m_blocked ? m_reason : QString()},
        {"flying", bool(m_train)},
        {"screens", int(QGuiApplication::screens().size())},
    };
    o.insert("text", statusText(m_set, o, true));
    return o;
}

QJsonObject Daemon::handle(const QByteArray &line) {
    const QList<QByteArray> parts = line.split(' ');
    const QByteArray cmd = parts.value(0);
    if (cmd == "ping" || cmd == "status") return status();
    if (cmd == "now") {
        m_set.sync();
        if (!m_set.enabled()) return {{"ok", false}, {"error", "Banners are switched off."}};
        const QString error = showNow();
        return error.isEmpty() ? QJsonObject{{"ok", true}} : QJsonObject{{"ok", false}, {"error", error}};
    }
    QJsonObject answer = command(cmd, parts);
    // Whoever changed something (the bar, the settings window, a key
    // binding), the plugin hears about it.
    publish();
    return answer;
}

QJsonObject Daemon::command(const QByteArray &cmd, const QList<QByteArray> &parts) {
    if (cmd == "pause") {
        const int minutes = std::clamp(parts.value(1, "60").toInt(), 1, 24 * 60);
        m_set.setPausedUntil(now() + minutes * 60'000LL);
        hide();
        schedule();
        return status();
    }
    if (cmd == "enable" || cmd == "disable") {
        m_set.setEnabled(cmd == "enable");
        reload();
        return status();
    }
    if (cmd == "resume") {
        m_set.setPausedUntil(0);
        schedule();
        return status();
    }
    if (cmd == "reload") {
        reload();
        return status();
    }
    if (cmd == "quit") {
        hide();
        return {{"ok", true}, {"quit", true}};
    }
    return {{"ok", false}, {"error", "Unknown command: " + QString::fromUtf8(cmd)}};
}
