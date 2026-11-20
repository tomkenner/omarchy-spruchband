// Open dialog through xdg-desktop-portal over D-Bus
// (org.freedesktop.portal.FileChooser), so the picker is the desktop's own.
#pragma once

#include <QObject>
#include <QVariantMap>

class PortalFilePicker : public QObject {
    Q_OBJECT
public:
    explicit PortalFilePicker(QObject *parent = nullptr) : QObject(parent) {}
    void open(const QString &title);

signals:
    void picked(const QString &path);
    void failed(const QString &message);

private slots:
    void onResponse(uint response, const QVariantMap &results);

private:
    void listen(const QString &path);
    void stopListening();
    QString m_requestPath;
};
