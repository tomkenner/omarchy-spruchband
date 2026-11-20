#include "settings.h"

#include "banners.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>

// ---------- Settings ----------
QString Settings::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/spruchband.conf";
}

Settings::Settings(const QString &path) : m_s(path, QSettings::IniFormat) {}

void Settings::set(const QString &key, const QVariant &v) {
    m_s.setValue(key, v);
    m_s.sync();
}

// An unticked-everything list is stored as one empty entry, so it reads back
// as empty instead of falling back to the defaults.
QStringList Settings::list(const QString &key, const QStringList &fallback) const {
    if (!m_s.contains(key)) return fallback;
    QStringList v = m_s.value(key).toStringList();
    v.removeAll(QString());
    return v;
}

QStringList Settings::figures() const {
    QStringList all;
    for (const Figure *f : Figures::all()) all << f->id;
    return list("figures", all);
}

QStringList Settings::styles() const { return list("styles", {"cartoon"}); }
QStringList Settings::collections() const { return list("collections", {"reminders"}); }

bool Settings::inQuietHours(const QTime &now) const {
    if (!quiet()) return false;
    const int h = now.hour(), a = quietFrom(), b = quietTo();
    if (a == b) return false;
    return a < b ? (h >= a && h < b) : (h >= a || h < b);
}

std::vector<Style> Settings::styleList() const {
    std::vector<Style> out;
    const QStringList ids = styles();
    for (const auto &s : allStyles())
        if (ids.contains(QLatin1String(s.id))) out.push_back(s.style);
    if (out.empty()) out.push_back(Style::Cartoon);
    return out;
}

std::vector<const Figure *> Settings::figureList() const {
    std::vector<const Figure *> out;
    const QStringList ids = figures();
    for (const Figure *f : Figures::all())
        if (ids.contains(f->id)) out.push_back(f);
    return out;
}

// ---------- Collections ----------
namespace {
QString &dirOverride() {
    static QString d;
    return d;
}
QString fileFor(const QString &id) { return Collections::dir() + "/" + id + ".txt"; }
int order(const QString &id) { return id == "reminders" ? 0 : 99; }
QString tr(const char *s) { return QCoreApplication::translate("Spruchband", s); }
}

namespace Collections {
QString dir() {
    const QString d = !dirOverride().isEmpty()
                        ? dirOverride()
                        : QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/spruchband/collections";
    QDir().mkpath(d);
    return d;
}

void setDir(const QString &d) { dirOverride() = d; }

void ensureDefaults() {
    QFile flag(dir() + "/.init");
    if (flag.exists()) return;
    // The English sample sayings are the translation keys, so the collection
    // starts out in the language Omarchy runs in.
    const char *lines[] = {"Drink a glass of water!", "Shoulders down, breathe deeply.", "Stand up and stretch.",
                           "Look away from the screen for 20 seconds.", "Have you laughed today?"};
    Coll c{"reminders", tr("Reminders"), {}};
    for (const char *l : lines) c.lines << tr(l);
    save(c);
    if (flag.open(QIODevice::WriteOnly)) flag.write("1");
}

QList<Coll> list() {
    QFileInfoList files = QDir(dir()).entryInfoList({"*.txt"}, QDir::Files);
    std::stable_sort(files.begin(), files.end(),
                     [](const QFileInfo &a, const QFileInfo &b) { return a.lastModified() < b.lastModified(); });
    std::stable_sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return order(a.completeBaseName()) < order(b.completeBaseName());
    });
    QList<Coll> out;
    for (const QFileInfo &f : files)
        if (auto c = load(f.completeBaseName())) out << *c;
    return out;
}

std::optional<Coll> load(const QString &id) {
    QFile f(fileFor(id));
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    QStringList all = QString::fromUtf8(f.readAll()).split('\n');
    QString name = id;
    if (!all.isEmpty() && all.first().startsWith("# ")) name = all.takeFirst().mid(2).trimmed();
    return Coll{id, clipText(name, MAX_NAME), parse(all.join('\n'))};
}

QStringList parse(const QString &text) {
    QStringList out;
    for (const QString &l : text.split('\n')) {
        const QString t = l.trimmed();
        if (!t.isEmpty() && !t.startsWith('#')) out << clipText(t);
    }
    return out;
}

bool save(const Coll &c) {
    // Written beside the target and renamed, so a crash never leaves half a file.
    QSaveFile f(fileFor(c.id));
    if (!f.open(QIODevice::WriteOnly)) return false;
    // The name stays on its line, whatever a file name brought along.
    const QString name = clipText(c.name.simplified(), MAX_NAME);
    f.write(("# " + name + "\n" + c.lines.join('\n') + "\n").toUtf8());
    return f.commit();
}

void remove(const QString &id) { QFile::remove(fileFor(id)); }

QString newId() { return "c" + QString::number(QDateTime::currentMSecsSinceEpoch(), 36); }

QStringList activeLines(const QStringList &active) {
    QStringList out;
    for (const Coll &c : list())
        if (active.contains(c.id)) out << c.lines;
    return out;
}
}

// ---------- Picker ----------
std::optional<QString> Picker::Bag::next(const QStringList &all) {
    if (all.isEmpty()) return std::nullopt;
    const size_t key = qHash(all);
    if (key != m_key || m_items.empty()) {
        m_key = key;
        QStringList shuffled = all;
        std::shuffle(shuffled.begin(), shuffled.end(), *QRandomGenerator::global());
        m_items.assign(shuffled.begin(), shuffled.end());
    }
    QString v = m_items.front();
    m_items.pop_front();
    return v;
}

std::optional<QString> Picker::text() { return m_texts.next(Collections::activeLines(m_set.collections())); }

const Figure *Picker::figure() {
    QStringList ids;
    for (const Figure *f : m_set.figureList()) ids << f->id;
    const auto id = m_figs.next(ids);
    return id ? Figures::byId(*id) : nullptr;
}

Style Picker::style() {
    QStringList ids;
    for (Style s : m_set.styleList()) ids << styleId(s);
    Style st = Style::Cartoon;
    if (const auto id = m_styles.next(ids)) styleFromId(*id, &st);
    return st;
}

std::vector<const Figure *> Picker::persons() {
    std::vector<const Figure *> out;
    for (const Figure *f : m_set.figureList())
        if (f->cat == Cat::Person) out.push_back(f);
    return out.empty() ? Figures::persons() : out;
}
