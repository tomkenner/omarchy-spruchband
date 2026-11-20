// Spruchband — every few minutes a figure pulls a saying across your screens.
//
//   spruchband                 the settings window
//   spruchband daemon [--stdio] the background service (the Omarchy plugin starts it
//                              with --stdio and talks to it over stdin and stdout)
//   spruchband now             a banner right now
//   spruchband pause [min]     pause (default 60 minutes)
//   spruchband resume          end the pause
//   spruchband toggle          pause for an hour, or resume
//   spruchband status [--json]

#include "backend.h"
#include "daemon.h"
#include "desk.h"
#include "ipc.h"
#include "preview.h"
#include "settings.h"
#include "status.h"
#include "theme.h"
#include "translator.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QLocalServer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <cstdio>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

static void setup(QCoreApplication &app) {
    app.setApplicationName("spruchband");
    app.setApplicationVersion(APP_VERSION);
}

static int usage(FILE *out) {
    fputs("Usage: spruchband [command]\n\n"
          "  (none)          open the settings\n"
          "  daemon          run the background service\n"
          "  now             show a banner right now\n"
          "  pause [min]     pause banners (default 60 minutes)\n"
          "  resume          end the pause\n"
          "  toggle          pause for an hour, or resume\n"
          "  status [--json] what the service is doing\n",
          out);
    return out == stderr ? 1 : 0;
}

static int runDaemon(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    setup(app);
    app.setQuitOnLastWindowClosed(false);
    Translator translator;
    app.installTranslator(&translator);
    Collections::ensureDefaults();
    Daemon daemon;
    if (!daemon.listen()) {
        fputs("spruchband: another service is running and would not stop\n", stderr);
        return 1;
    }
    if (app.arguments().contains("--stdio")) daemon.attachStdio();
    return app.exec();
}

static int runCli(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    setup(app);
    Translator translator;
    app.installTranslator(&translator);
    const QStringList args = app.arguments().mid(1);
    const QString cmd = args.value(0);
    const bool json = args.contains("--json");
    Settings set;

    auto print = [&](const QJsonObject &o, const QString &text) {
        if (json) fputs(QJsonDocument(o).toJson(QJsonDocument::Compact).append('\n').constData(), stdout);
        else if (!text.isEmpty()) fputs((text + "\n").toUtf8().constData(), stdout);
    };
    auto fail = [&](const QString &error) {
        if (json) print({{"ok", false}, {"error", error}}, {});
        fputs(("spruchband: " + error + "\n").toUtf8().constData(), stderr);
        return 1;
    };

    if (cmd == "now") {
        const auto r = Ipc::send("now");
        if (!r) return fail("the background service isn't running; it starts with the Omarchy plugin "
                            "(omarchy plugin enable tkenner.spruchband)");
        if (!r->value("ok").toBool()) return fail(r->value("error").toString());
        print(*r, {});
        return 0;
    }

    QString pauseCmd = cmd;
    if (cmd == "toggle") pauseCmd = set.paused() ? "resume" : "pause";
    if (pauseCmd == "pause" || pauseCmd == "resume") {
        int minutes = 0;
        if (pauseCmd == "pause") {
            bool ok = true;
            const QString arg = cmd == "pause" && args.size() > 1 && args[1] != "--json" ? args[1] : QString("60");
            minutes = arg.toInt(&ok);
            if (!ok || minutes <= 0) return fail("pause takes a number of minutes");
            minutes = std::min(minutes, 24 * 60);
        }
        // Settings work without the service; it is told when it runs.
        set.setPausedUntil(minutes ? QDateTime::currentMSecsSinceEpoch() + minutes * 60'000LL : 0);
        const auto r = Ipc::send(pauseCmd == "pause" ? "pause " + QByteArray::number(minutes) : "resume");
        const QString text = minutes ? QString("Paused for %1 minutes.").arg(minutes) : QString("Resumed.");
        print(r.value_or(QJsonObject{{"ok", true}, {"running", false}, {"paused", minutes > 0}}), text);
        return 0;
    }

    if (cmd == "status") {
        const auto r = Ipc::send("status");
        if (!r) {
            const QString text = statusText(set, {}, false);
            print({{"ok", true}, {"running", false}, {"enabled", set.enabled()}, {"paused", set.paused()}, {"text", text}}, text);
            return 0;
        }
        print(*r, r->value("text").toString());
        return 0;
    }
    return usage(stderr);
}

static int runUi(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    setup(app);
    // Matches the window to spruchband.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName("spruchband");
    app.setWindowIcon(QIcon::fromTheme("spruchband"));

    // One settings window: a second launch focuses the first instead.
    QLocalServer single;
    const QString uiSocket = Ipc::socketPath("spruchband-ui");
    {
        QLocalSocket probe;
        probe.connectToServer(uiSocket);
        if (probe.waitForConnected(300)) {
            Desk::hyprRequest("dispatch focuswindow class:^(spruchband)$");
            return 0;
        }
    }
    QLocalServer::removeServer(uiSocket);
    single.listen(uiSocket);

    QQuickStyle::setStyle("Material");
    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);
    Collections::ensureDefaults();

    Theme theme;
    Backend backend;
    qmlRegisterType<StagePreview>("Spruchband", 1, 0, "StagePreview");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}

int main(int argc, char *argv[]) {
    const QString cmd = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    if (cmd == "daemon") return runDaemon(argc, argv);
    if (cmd == "now" || cmd == "pause" || cmd == "resume" || cmd == "toggle" || cmd == "status")
        return runCli(argc, argv);
    if (cmd == "-h" || cmd == "--help" || cmd == "help") return usage(stdout);
    if (cmd == "-v" || cmd == "--version") {
        puts(APP_VERSION);
        return 0;
    }
    if (!cmd.isEmpty() && cmd != "open") return usage(stderr);
    return runUi(argc, argv);
}
