// Settings, collections and the random picker, ported from the Android app's
// data/Settings.kt and data/Picker.kt. Settings live in ~/.config/spruchband.conf
// (shared by the settings window, the background service and the command
// line); collections are text files in ~/.local/share/spruchband/collections.
#pragma once

#include "figures.h"

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <algorithm>
#include <deque>
#include <optional>

class Settings {
public:
    explicit Settings(const QString &path = defaultPath());
    static QString defaultPath();

    // Re-reads what other processes wrote.
    void sync() { m_s.sync(); }

    bool enabled() const { return m_s.value("enabled", true).toBool(); }
    void setEnabled(bool v) { set("enabled", v); }
    int interval() const { return std::clamp(m_s.value("interval", 10).toInt(), 1, 60); }
    void setInterval(int v) { set("interval", v); }
    bool jitter() const { return m_s.value("jitter", true).toBool(); }
    void setJitter(bool v) { set("jitter", v); }
    QStringList figures() const;
    void setFigures(const QStringList &v) { set("figures", v.isEmpty() ? QStringList{QString()} : v); }
    QStringList styles() const;
    void setStyles(const QStringList &v) { set("styles", v.isEmpty() ? QStringList{QString()} : v); }
    QStringList collections() const;
    void setCollections(const QStringList &v) { set("collections", v.isEmpty() ? QStringList{QString()} : v); }
    int size() const { return m_s.value("size", 88).toInt(); }
    void setSize(int v) { set("size", v); }
    int band() const { return m_s.value("band", 36).toInt(); }
    void setBand(int v) { set("band", v); }
    int speed() const { return m_s.value("speed", 220).toInt(); }
    void setSpeed(int v) { set("speed", v); }
    int direction() const { return m_s.value("dir", 0).toInt(); }
    void setDirection(int v) { set("dir", v); }
    bool quiet() const { return m_s.value("quiet", true).toBool(); }
    void setQuiet(bool v) { set("quiet", v); }
    int quietFrom() const { return m_s.value("quietFrom", 22).toInt(); }
    void setQuietFrom(int v) { set("quietFrom", v); }
    int quietTo() const { return m_s.value("quietTo", 7).toInt(); }
    void setQuietTo(int v) { set("quietTo", v); }
    bool respectDnd() const { return m_s.value("dnd", true).toBool(); }
    void setRespectDnd(bool v) { set("dnd", v); }
    bool allScreens() const { return m_s.value("allScreens", true).toBool(); }
    void setAllScreens(bool v) { set("allScreens", v); }
    qint64 pausedUntil() const { return m_s.value("pausedUntil", 0).toLongLong(); }
    void setPausedUntil(qint64 v) { set("pausedUntil", v); }

    bool paused() const { return pausedUntil() > QDateTime::currentMSecsSinceEpoch(); }
    bool inQuietHours(const QTime &now = QTime::currentTime()) const;
    std::vector<Style> styleList() const;
    std::vector<const Figure *> figureList() const;

private:
    void set(const QString &key, const QVariant &v);
    QStringList list(const QString &key, const QStringList &fallback) const;
    mutable QSettings m_s;
};

// One collection of sayings: collections/<id>.txt, first line "# Name".
struct Coll {
    QString id;
    QString name;
    QStringList lines;
};

namespace Collections {
QString dir();
void setDir(const QString &dir); // tests
// Creates the sample collection in the current language on first start.
void ensureDefaults();
QList<Coll> list();
std::optional<Coll> load(const QString &id);
QStringList parse(const QString &text);
bool save(const Coll &c);
void remove(const QString &id);
QString newId();
// All sayings of the active collections.
QStringList activeLines(const QStringList &active);
}

// Picks saying, figure and style. Shuffle bags: everything comes up once
// before anything repeats.
class Picker {
public:
    explicit Picker(Settings &set) : m_set(set) {}
    std::optional<QString> text();
    const Figure *figure();
    Style style();
    std::vector<const Figure *> persons();

private:
    class Bag {
    public:
        std::optional<QString> next(const QStringList &all);

    private:
        std::deque<QString> m_items;
        size_t m_key = 0;
    };
    Settings &m_set;
    Bag m_texts, m_figs, m_styles;
};
