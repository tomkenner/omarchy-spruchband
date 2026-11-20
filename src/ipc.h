// The background service listens on a local socket; the bar plugin, the
// settings window and the command line send it one-line commands and get one
// line of JSON back.
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace Ipc {
QString socketPath(const QString &name = "spruchband");
// Nothing when the service isn't running.
std::optional<QJsonObject> send(const QByteArray &command, int timeoutMs = 1000);
}
