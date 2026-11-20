#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "desk.h"
#include "settings.h"
#include "stage.h"
#include "theme.h"
#include "translator.h"

class SpruchbandTests : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QVERIFY(m_dir.isValid());
        Collections::setDir(m_dir.path() + "/collections");
    }

    void collectionsRoundTrip() {
        QCOMPARE(Collections::parse("  a \n\n# comment\nb\n"), QStringList({"a", "b"}));
        QVERIFY(Collections::save({"c1", "Mine", {"one", "two"}}));
        const auto c = Collections::load("c1");
        QVERIFY(c.has_value());
        QCOMPARE(c->name, QString("Mine"));
        QCOMPARE(c->lines, QStringList({"one", "two"}));
        Collections::remove("c1");
        QVERIFY(!Collections::load("c1").has_value());
    }

    void defaultsComeFirstAndOnce() {
        Collections::ensureDefaults();
        QVERIFY(Collections::save({"cz", "Later", {"x"}}));
        const auto list = Collections::list();
        QCOMPARE(list.first().id, QString("reminders"));
        QCOMPARE(list.first().lines.size(), 5);
        Collections::remove("reminders");
        Collections::ensureDefaults(); // the flag keeps a deleted sample deleted
        QVERIFY(!Collections::load("reminders").has_value());
    }

    void settingsKeepAnEmptyList() {
        Settings set(m_dir.path() + "/s.conf");
        QCOMPARE(set.styles(), QStringList({"cartoon"}));
        set.setFigures({});
        QVERIFY(set.figures().isEmpty());
        QVERIFY(set.figureList().empty());
        set.setStyles({"pixel", "neon"});
        Settings again(m_dir.path() + "/s.conf");
        QCOMPARE(again.styles(), QStringList({"pixel", "neon"}));
    }

    void quietHoursWrapMidnight() {
        Settings set(m_dir.path() + "/q.conf");
        set.setQuiet(true);
        set.setQuietFrom(22);
        set.setQuietTo(7);
        QVERIFY(set.inQuietHours(QTime(23, 0)));
        QVERIFY(set.inQuietHours(QTime(6, 59)));
        QVERIFY(!set.inQuietHours(QTime(7, 0)));
        QVERIFY(!set.inQuietHours(QTime(12, 0)));
        set.setQuiet(false);
        QVERIFY(!set.inQuietHours(QTime(23, 0)));
    }

    void pickerShowsEverySayingBeforeRepeating() {
        Settings set(m_dir.path() + "/p.conf");
        QVERIFY(Collections::save({"pk", "P", {"a", "b", "c", "d"}}));
        set.setCollections({"pk"});
        Picker picker(set);
        QSet<QString> seen;
        for (int i = 0; i < 4; i++) seen.insert(*picker.text());
        QCOMPARE(seen.size(), 4);
    }

    // Every figure in every style draws something and finishes its flight.
    void everyStyleAndFigureRenders() {
        Stage stage(1);
        const int W = 700;
        for (const auto &si : allStyles()) {
            for (const Figure *f : Figures::all()) {
                Train tr = stage.makeTrain(*f, "Hallo Welt!", si.style, 88, 36, 220, 1, W, Figures::persons(),
                                           [] { return 0.4f; });
                QVERIFY(tr.x0 < 0 && tr.x1 > W);
                QImage img(W, int(tr.height()) + 1, QImage::Format_ARGB32_Premultiplied);
                img.fill(Qt::transparent);
                {
                    QPainter p(&img);
                    QVERIFY(!stage.render(p, W, img.height(), tr, (W * 0.6f - tr.x0) / tr.v, tr.up));
                }
                QVERIFY2(opaquePixels(img) > 200, qPrintable(QString("%1 / %2").arg(si.id, f->id)));
                QImage end(W, img.height(), QImage::Format_ARGB32_Premultiplied);
                QPainter p(&end);
                QVERIFY(stage.render(p, W, end.height(), tr, tr.duration() + 0.1f, tr.up));
            }
        }
    }

    // Two screens side by side show the same picture as one wide screen.
    void splitScreensMatchOneWideScreen() {
        Stage a(1), b(1), whole(1);
        const int W = 1200, half = 600;
        Train tr = whole.makeTrain(*Figures::byId("flugzeug"), "Trink ein Glas Wasser!", Style::Cartoon, 88, 36, 220, 1, W,
                                   {}, [] { return 0.2f; });
        const int h = int(tr.height()) + 1;
        const float el = (W * 0.62f - tr.x0) / tr.v; // the banner straddles the seam
        QImage full(W, h, QImage::Format_ARGB32_Premultiplied), left(half, h, QImage::Format_ARGB32_Premultiplied),
            right(half, h, QImage::Format_ARGB32_Premultiplied);
        for (QImage *img : {&full, &left, &right}) img->fill(Qt::transparent);
        { QPainter p(&full); whole.render(p, W, h, tr, el, tr.up, 0); }
        { QPainter p(&left); a.render(p, half, h, tr, el, tr.up, 0); }
        { QPainter p(&right); b.render(p, half, h, tr, el, tr.up, half); }
        QVERIFY(opaquePixels(left) > 100 && opaquePixels(right) > 100);
        int differ = 0;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < W; x++) {
                const QRgb s = x < half ? left.pixel(x, y) : right.pixel(x - half, y);
                if (std::abs(qAlpha(s) - qAlpha(full.pixel(x, y))) > 24) differ++;
            }
        QVERIFY2(differ < W * h / 1000, qPrintable(QString::number(differ)));
    }

    void hyprlandStateIsRead() {
        const QByteArray monitors = R"([
          {"name":"eDP-1","reserved":[0,26,0,0],"focused":true,"dpmsStatus":true,
           "activeWorkspace":{"id":1},"specialWorkspace":{"id":0},"solitaryBlockedBy":["WINDOWED"]},
          {"name":"HDMI-A-1","reserved":[0,0,0,0],"focused":false,"dpmsStatus":true,
           "activeWorkspace":{"id":11},"specialWorkspace":{"id":0},"solitaryBlockedBy":[]}])";
        DeskState d = Desk::parse(monitors, R"([{"workspace":{"id":11},"fullscreen":1,"mapped":true}])");
        QVERIFY(d.hyprland);
        QCOMPARE(d.focused, QString("eDP-1"));
        QCOMPARE(d.monitors.value("eDP-1").reservedTop, 26);
        QVERIFY(!d.fullscreen); // maximised is not full screen
        QVERIFY(!d.locked);
        d = Desk::parse(monitors, R"([{"workspace":{"id":11},"fullscreen":2,"mapped":true}])");
        QVERIFY(d.fullscreen);
        d = Desk::parse(monitors, R"([{"workspace":{"id":5},"fullscreen":2,"mapped":true}])");
        QVERIFY(!d.fullscreen); // on a workspace nobody sees
        QByteArray locked = monitors;
        locked.replace(R"("solitaryBlockedBy":[])", R"("solitaryBlockedBy":["LOCK"])");
        QVERIFY(Desk::parse(locked, "[]").locked);
        QByteArray off = monitors;
        off.replace(R"("dpmsStatus":true)", R"("dpmsStatus":false)");
        QVERIFY(Desk::parse(off, "[]").screensOff);
    }

    void omarchyDoNotDisturb() {
        const QString path = m_dir.path() + "/quiet.json";
        QVERIFY(!Desk::dnd(path + ".missing"));
        write(path, R"({"dnd":true})");
        QVERIFY(Desk::dnd(path));
        write(path, R"({"dnd":false})");
        QVERIFY(!Desk::dnd(path));
    }

    void importRefusesBinaryFiles() {
        Settings set(m_dir.path() + "/i.conf");
        Backend backend(&set);
        write(m_dir.path() + "/Gute Laune.txt", "Lächeln!\n\nKaffee?\n");
        QVERIFY(backend.importFile(m_dir.path() + "/Gute Laune.txt"));
        bool found = false;
        for (const Coll &c : Collections::list())
            if (c.name == "Gute Laune") {
                found = true;
                QCOMPARE(c.lines, QStringList({"Lächeln!", "Kaffee?"}));
                QVERIFY(set.collections().contains(c.id));
            }
        QVERIFY(found);
        write(m_dir.path() + "/x.png", QByteArray("\x89PNG\0\0", 6));
        QVERIFY(!backend.importFile(m_dir.path() + "/x.png"));
    }

    void translatorFollowsOmarchyLanguage() {
        write(m_dir.path() + "/language", "de\n");
        Translator translator(m_dir.path() + "/language", ":/i18n-missing");
        QCOMPARE(translator.language(), QString("de"));
    }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
    }

private:
    static int opaquePixels(const QImage &img) {
        int n = 0;
        for (int y = 0; y < img.height(); y++)
            for (int x = 0; x < img.width(); x++)
                if (qAlpha(img.pixel(x, y)) > 128) n++;
        return n;
    }
    static void write(const QString &path, const QByteArray &data) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }
    QTemporaryDir m_dir;
};

QTEST_MAIN(SpruchbandTests)
#include "spruchband_tests.moc"
