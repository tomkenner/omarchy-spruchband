#include "ipc.h"

#include <QDir>
#include <QJsonDocument>
#include <QLocalSocket>
#include <unistd.h>

namespace Ipc {

QString socketPath(const QString &name) {
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (!runtime.isEmpty()) return runtime + "/" + name + ".sock";
    return QDir::tempPath() + "/" + name + "-" + QString::number(getuid()) + ".sock";
}

std::optional<QJsonObject> send(const QByteArray &command, int timeoutMs) {
    QLocalSocket s;
    s.connectToServer(socketPath());
    if (!s.waitForConnected(timeoutMs)) return std::nullopt;
    s.write(command + "\n");
    if (!s.waitForBytesWritten(timeoutMs)) return std::nullopt;
    QByteArray answer;
    while (!answer.contains('\n') && s.waitForReadyRead(timeoutMs)) answer += s.readAll();
    if (answer.isEmpty()) return std::nullopt;
    return QJsonDocument::fromJson(answer.trimmed()).object();
}
}
