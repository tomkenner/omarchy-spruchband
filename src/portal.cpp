#include "portal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QRandomGenerator>
#include <QUrl>

static const char *SERVICE = "org.freedesktop.portal.Desktop";

void PortalFilePicker::open(const QString &title) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit failed(tr("No file picker is available (xdg-desktop-portal is not running)."));
        return;
    }
    // The portal names the request after our bus name and this token, so we
    // can listen for its answer before asking and never miss it.
    const QString token = "spruchband" + QString::number(QRandomGenerator::global()->generate());
    const QString sender = bus.baseService().mid(1).replace('.', '_');
    listen("/org/freedesktop/portal/desktop/request/" + sender + "/" + token);

    QDBusMessage call = QDBusMessage::createMethodCall(SERVICE, "/org/freedesktop/portal/desktop",
                                                       "org.freedesktop.portal.FileChooser", "OpenFile");
    call << QString() << title << QVariantMap{{"handle_token", token}, {"modal", true}};
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        const QDBusPendingReply<QDBusObjectPath> reply = *w;
        if (reply.isError()) {
            stopListening();
            emit failed(tr("No file picker is available (xdg-desktop-portal is not running)."));
        } else if (reply.value().path() != m_requestPath) {
            // Old portals ignore the token and pick their own path.
            stopListening();
            listen(reply.value().path());
        }
    });
}

void PortalFilePicker::listen(const QString &path) {
    m_requestPath = path;
    QDBusConnection::sessionBus().connect(SERVICE, path, "org.freedesktop.portal.Request", "Response", this,
                                          SLOT(onResponse(uint, QVariantMap)));
}

void PortalFilePicker::stopListening() {
    if (m_requestPath.isEmpty()) return;
    QDBusConnection::sessionBus().disconnect(SERVICE, m_requestPath, "org.freedesktop.portal.Request", "Response", this,
                                             SLOT(onResponse(uint, QVariantMap)));
    m_requestPath.clear();
}

void PortalFilePicker::onResponse(uint response, const QVariantMap &results) {
    stopListening();
    if (response != 0) return; // cancelled
    const QStringList uris = results.value("uris").toStringList();
    if (!uris.isEmpty()) emit picked(QUrl(uris.first()).toLocalFile());
}
