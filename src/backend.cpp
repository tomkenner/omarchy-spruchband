#include "backend.h"

#include "banners.h"
#include "ipc.h"
#include "status.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QUrl>
#include <cmath>

Backend::Backend(Settings *settings, QObject *parent)
    : QObject(parent), m_set(settings ? settings : &m_ownSettings) {
    m_reload.setSingleShot(true);
    m_reload.setInterval(250);
    connect(&m_reload, &QTimer::timeout, this, [] { Ipc::send("reload", 500); });
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &Backend::refreshStatus);
    connect(&m_picker, &PortalFilePicker::picked, this, [this](const QString &path) { importFile(path); });
    connect(&m_picker, &PortalFilePicker::failed, this, &Backend::message);
    if (!settings) {
        // The real window follows the service the plugin runs.
        refreshStatus();
        m_poll.start();
    }
}

void Backend::setEnabled(bool v) {
    write([&] { m_set->setEnabled(v); });
    Ipc::send(v ? "enable" : "disable", 500);
    refreshStatus();
}

QStringList Backend::toggled(QStringList list, const QString &id, bool on) {
    list.removeAll(id);
    if (on) list << id;
    return list;
}

void Backend::setFigure(const QString &id, bool on) { write([&] { m_set->setFigures(toggled(m_set->figures(), id, on)); }); }

void Backend::setCategory(const QString &cat, bool on) {
    QStringList list = m_set->figures();
    for (const Figure *f : Figures::all())
        for (const auto &c : allCats())
            if (c.cat == f->cat && cat == QLatin1String(c.id)) list = toggled(list, f->id, on);
    write([&] { m_set->setFigures(list); });
}

void Backend::setStyle(const QString &id, bool on) { write([&] { m_set->setStyles(toggled(m_set->styles(), id, on)); }); }

void Backend::setCollectionActive(const QString &id, bool on) {
    write([&] { m_set->setCollections(toggled(m_set->collections(), id, on)); });
}

QVariantList Backend::collections() const {
    QVariantList out;
    for (const Coll &c : Collections::list())
        out << QVariantMap{{"id", c.id}, {"name", c.name}, {"count", int(c.lines.size())}};
    return out;
}

QVariantList Backend::categoryModel() const {
    QVariantList out;
    for (const auto &c : allCats()) out << QVariantMap{{"id", c.id}, {"name", c.name}};
    return out;
}

QVariantList Backend::figureModel() const {
    QVariantList out;
    for (const Figure *f : Figures::all()) {
        QString cat;
        for (const auto &c : allCats())
            if (c.cat == f->cat) cat = c.id;
        out << QVariantMap{{"id", f->id}, {"name", f->name}, {"cat", cat}, {"kind", BannerKind::of(*f).name()}};
    }
    return out;
}

QVariantList Backend::styleModel() const {
    QVariantList out;
    for (const auto &s : allStyles()) out << QVariantMap{{"id", s.id}, {"name", s.name}, {"hint", s.hint}};
    return out;
}

QString Backend::version() const { return QCoreApplication::applicationVersion(); }

void Backend::refreshStatus() {
    const auto st = Ipc::send("status", 300);
    m_running = st.has_value();
    m_status = st.value_or(QJsonObject());
    emit statusChanged();
}

QString Backend::statusText() const { return ::statusText(*m_set, m_status, m_running); }

QVariantMap Backend::loadCollection(const QString &id) const {
    const auto c = Collections::load(id);
    if (!c) return {};
    return {{"name", c->name}, {"text", c->lines.join('\n')}};
}

QString Backend::saveCollection(const QString &id, const QString &name, const QString &text) {
    const bool fresh = id.isEmpty();
    Coll c{fresh ? Collections::newId() : id, name.trimmed(), Collections::parse(text)};
    if (c.name.isEmpty()) c.name = tr("My sayings");
    if (!Collections::save(c)) {
        emit message(tr("Could not save the collection."));
        return {};
    }
    if (fresh) write([&] { m_set->setCollections(toggled(m_set->collections(), c.id, true)); });
    emit collectionsChanged();
    emit message(tr("Saved"));
    m_reload.start();
    return c.id;
}

void Backend::deleteCollection(const QString &id) {
    Collections::remove(id);
    write([&] { m_set->setCollections(toggled(m_set->collections(), id, false)); });
    emit collectionsChanged();
}

void Backend::importCollection() { m_picker.open(tr("Choose a text file")); }

bool Backend::importFile(const QString &path) {
    QFile f(path);
    const QByteArray data = f.open(QIODevice::ReadOnly) ? f.read(4 * 1024 * 1024) : QByteArray();
    // Binary files (images, PDFs) are refused rather than turned into noise.
    if (data.isEmpty() || data.contains('\0')) {
        emit message(tr("This file could not be read. Please choose a plain text file."));
        return false;
    }
    const QString name = QFileInfo(path).completeBaseName();
    Coll c{Collections::newId(), name, Collections::parse(QString::fromUtf8(data))};
    if (!Collections::save(c)) {
        emit message(tr("Could not save the collection."));
        return false;
    }
    write([&] { m_set->setCollections(toggled(m_set->collections(), c.id, true)); });
    emit collectionsChanged();
    emit message(tr("Imported \"%1\" with %2 sayings").arg(name).arg(c.lines.size()));
    return true;
}

void Backend::openFolder() { QDesktopServices::openUrl(QUrl::fromLocalFile(Collections::dir())); }

void Backend::showNow() {
    if (!m_set->enabled()) {
        emit message(tr("Switch on “Show banners” first."));
        return;
    }
    const auto r = Ipc::send("now", 1500);
    if (!r) emit message(statusText());
    else if (!r->value("ok").toBool()) emit message(tr(r->value("error").toString().toUtf8().constData()));
    refreshStatus();
}

void Backend::pause(int minutes) {
    if (!Ipc::send("pause " + QByteArray::number(minutes), 500))
        m_set->setPausedUntil(QDateTime::currentMSecsSinceEpoch() + minutes * 60'000LL);
    m_set->sync();
    refreshStatus();
}

void Backend::resume() {
    if (!Ipc::send("resume", 500)) m_set->setPausedUntil(0);
    m_set->sync();
    refreshStatus();
}
