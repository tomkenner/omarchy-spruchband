#include "status.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <cmath>

static QString tr(const char *s, int n = -1) { return QCoreApplication::translate("Spruchband", s, nullptr, n); }

QString statusText(const Settings &set, const QJsonObject &status, bool running) {
    if (!running) return tr("The background service isn't running. It starts with the Spruchband plugin in the Omarchy bar.");
    if (!set.enabled()) return tr("Banners are switched off.");
    if (set.paused()) {
        const QTime until = QDateTime::fromMSecsSinceEpoch(set.pausedUntil()).time();
        return tr("Paused until %1.").arg(QLocale().toString(until, QLocale::ShortFormat));
    }
    if (status.value("flying").toBool()) return tr("A banner is flying right now.");
    const QString blocked = status.value("blocked").toString();
    if (set.inQuietHours() || blocked == "quiet") return tr("Waiting: quiet hours.");
    if (blocked == "dnd") return tr("Waiting: Do Not Disturb is on.");
    if (blocked == "fullscreen") return tr("Waiting: a full-screen app is running.");
    if (blocked == "locked" || blocked == "screensOff") return tr("Waiting: the screen is locked or off.");
    const qint64 next = qint64(status.value("nextAt").toDouble()) - QDateTime::currentMSecsSinceEpoch();
    if (next < 60'000) return tr("Next banner in less than a minute.");
    return tr("Next banner in about %n minutes", int(std::ceil(next / 60'000.0)));
}
