// The settings window's state, handed to QML as `backend`. Every change is
// written to the shared settings file and the background service (run by the
// Omarchy plugin) is told to reload; its status is polled while the window is
// open.
#pragma once

#include "portal.h"
#include "settings.h"

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVariantList>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY changed)
    Q_PROPERTY(bool jitter READ jitter WRITE setJitter NOTIFY changed)
    Q_PROPERTY(bool respectDnd READ respectDnd WRITE setRespectDnd NOTIFY changed)
    Q_PROPERTY(bool quiet READ quiet WRITE setQuiet NOTIFY changed)
    Q_PROPERTY(int quietFrom READ quietFrom WRITE setQuietFrom NOTIFY changed)
    Q_PROPERTY(int quietTo READ quietTo WRITE setQuietTo NOTIFY changed)
    Q_PROPERTY(int figureSize READ figureSize WRITE setFigureSize NOTIFY changed)
    Q_PROPERTY(int bandHeight READ bandHeight WRITE setBandHeight NOTIFY changed)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY changed)
    Q_PROPERTY(int direction READ direction WRITE setDirection NOTIFY changed)
    Q_PROPERTY(bool allScreens READ allScreens WRITE setAllScreens NOTIFY changed)
    Q_PROPERTY(QStringList figures READ figures NOTIFY changed)
    Q_PROPERTY(QStringList styles READ styles NOTIFY changed)
    Q_PROPERTY(QStringList activeCollections READ activeCollections NOTIFY changed)
    Q_PROPERTY(QVariantList collections READ collections NOTIFY collectionsChanged)
    Q_PROPERTY(QVariantList categoryModel READ categoryModel CONSTANT)
    Q_PROPERTY(QVariantList figureModel READ figureModel CONSTANT)
    Q_PROPERTY(QVariantList styleModel READ styleModel CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY statusChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit Backend(Settings *settings = nullptr, QObject *parent = nullptr);

    bool enabled() const { return m_set->enabled(); }
    void setEnabled(bool v);
    int interval() const { return m_set->interval(); }
    void setInterval(int v) { write([&] { m_set->setInterval(v); }); }
    bool jitter() const { return m_set->jitter(); }
    void setJitter(bool v) { write([&] { m_set->setJitter(v); }); }
    bool respectDnd() const { return m_set->respectDnd(); }
    void setRespectDnd(bool v) { write([&] { m_set->setRespectDnd(v); }); }
    bool quiet() const { return m_set->quiet(); }
    void setQuiet(bool v) { write([&] { m_set->setQuiet(v); }); }
    int quietFrom() const { return m_set->quietFrom(); }
    void setQuietFrom(int v) { write([&] { m_set->setQuietFrom(v); }); }
    int quietTo() const { return m_set->quietTo(); }
    void setQuietTo(int v) { write([&] { m_set->setQuietTo(v); }); }
    int figureSize() const { return m_set->size(); }
    void setFigureSize(int v) { write([&] { m_set->setSize(v); }); }
    int bandHeight() const { return m_set->band(); }
    void setBandHeight(int v) { write([&] { m_set->setBand(v); }); }
    int speed() const { return m_set->speed(); }
    void setSpeed(int v) { write([&] { m_set->setSpeed(v); }); }
    int direction() const { return m_set->direction(); }
    void setDirection(int v) { write([&] { m_set->setDirection(v); }); }
    bool allScreens() const { return m_set->allScreens(); }
    void setAllScreens(bool v) { write([&] { m_set->setAllScreens(v); }); }
    QStringList figures() const { return m_set->figures(); }
    QStringList styles() const { return m_set->styles(); }
    QStringList activeCollections() const { return m_set->collections(); }
    QVariantList collections() const;
    QVariantList categoryModel() const;
    QVariantList figureModel() const;
    QVariantList styleModel() const;
    bool running() const { return m_running; }
    bool paused() const { return m_set->paused(); }
    QString statusText() const;
    QString version() const;

    Q_INVOKABLE void setFigure(const QString &id, bool on);
    Q_INVOKABLE void setCategory(const QString &cat, bool on);
    Q_INVOKABLE void setStyle(const QString &id, bool on);
    Q_INVOKABLE void setCollectionActive(const QString &id, bool on);
    Q_INVOKABLE QVariantMap loadCollection(const QString &id) const;
    // Returns the collection's id, or "" when it could not be saved.
    Q_INVOKABLE QString saveCollection(const QString &id, const QString &name, const QString &text);
    Q_INVOKABLE void deleteCollection(const QString &id);
    Q_INVOKABLE void importCollection();
    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void showNow();
    Q_INVOKABLE void pause(int minutes);
    Q_INVOKABLE void resume();
    Q_INVOKABLE void refreshStatus();

    // Imports a text file as a new collection (also used by tests).
    bool importFile(const QString &path);

signals:
    void changed();
    void collectionsChanged();
    void statusChanged();
    void message(const QString &text);

private:
    template <typename F> void write(F f) {
        f();
        emit changed();
        m_reload.start();
    }
    static QStringList toggled(QStringList list, const QString &id, bool on);

    Settings m_ownSettings;
    Settings *m_set;
    PortalFilePicker m_picker;
    QTimer m_reload, m_poll;
    bool m_running = false;
    QJsonObject m_status;
};
