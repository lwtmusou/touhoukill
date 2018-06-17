#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>

#include "engine.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
#if defined(Q_OS_WIN)
    // QDir::setCurrent(a.applicationDirPath());
#elif defined(Q_OS_LINUX)
#ifdef Q_OS_ANDROID
    // TODO: copy all the files from qrc(or assets) to /sdcard
    QDir::setCurrent("/sdcard/TouhouSatsu");
#else
    // TODO: in prefixed build, assets shoule be read from /usr/share/TouhouSatsu, configs should be written to /etc
#endif
#elif defined(Q_OS_MACOS)
    // TODO: assets should be read from app bundle, configs should be written to the folder where the app bundle is lying
#elif defined(Q_OS_IOS)
    // wait for an expert in IOS
#else
#error "TouhouSatsu is not supperted on this platform."
#endif

    qsrand(QDateTime::currentMSecsSinceEpoch());

    QTranslator qtTranslator;
    QTranslator qSgsTranslator;
    qtTranslator.load("qt_zh_CN.qm");
    qSgsTranslator.load("sanguosha.qm");

    a.installTranslator(&qtTranslator);
    a.installTranslator(&qSgsTranslator);

    Sanguosha = new Engine;
    Config.init();

    a.setFont(Config.font());

    QQmlApplicationEngine appEngine;
    appEngine.rootContext()->setContextProperty("Sanguosha", Sanguosha);
    appEngine.rootContext()->setContextProperty("Config", &Config);
    appEngine.load("qml/MainWindow.qml");

    return a.exec();
}

#if 0

#include "audio.h"
#include "legacyserver.h"
#include "mainwindow.h"
#include "settings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStyleFactory>
#include <QTranslator>

int main(int argc, char *argv[])
{
    // refactor proposal: do not check the first argument only!!!
    // note: QSanguoshaServer will be separated from main program, so we will be able to just 'execve' to QSanguoshaServer and we're done.
    if (argc > 1 && strcmp(argv[1], "-server") == 0) {
        new QCoreApplication(argc, argv);
    } else {
        new QApplication(argc, argv);
        QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + QStringLiteral("/plugins"));

#ifndef Q_OS_WIN32
        if (QStyleFactory::keys().contains(QStringLiteral("Fusion"), Qt::CaseInsensitive))
            qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
#endif
    }

#ifdef Q_OS_ANDROID
    QDir::setCurrent(QStringLiteral("/sdcard/Android/data/rocks.touhousatsu.app"));
    // extract data from assets
#else
    // Deal with package manager later
    // e.g.: in Linux distributions and Windows, we should put our data files to /prefix/share
#endif

    QTranslator qt_translator;
    QTranslator translator;

    if (qt_translator.load(QStringLiteral("qt_zh_CN.qm")))
        QCoreApplication::installTranslator(&qt_translator);
    else
        qDebug() << "Unable to load qt_zh_CN.qm";

    if (translator.load(QStringLiteral("sanguosha.qm")))
        QCoreApplication::installTranslator(&translator);
    else
        qDebug() << "Unable to load sanguosha.qm";

    // initialize Engine;
    Sanguosha->init();

    Config.init();

    // refactor proposal: using QCommandLineParser
    if (QCoreApplication::arguments().contains(QStringLiteral("-server"))) {
        LegacyServer *server = new LegacyServer(QCoreApplication::instance());
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen())
            printf("Starting successfully\n");
        else
            printf("Starting failed!\n");

        int r = QCoreApplication::exec();
        delete QCoreApplication::instance();
        return r;
    }

    QFile file(QStringLiteral("sanguosha.qss"));
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        qApp->setStyleSheet(stream.readAll());
    }

    qApp->setFont(Config.AppFont);

    MainWindow *main_window = new MainWindow;

    main_window->show();

    // refactor proposal: using QCommandLineParser
    foreach (QString arg, QCoreApplication::instance()->arguments()) {
        if (arg.startsWith(QStringLiteral("-connect:"))) {
            arg.remove(QStringLiteral("-connect:"));
            Config.HostAddress = arg;
            Config.setValue(QStringLiteral("HostUrl"), arg);

            main_window->startConnection();
            break;
        }
    }

    int execResult = QCoreApplication::exec();
    delete QCoreApplication::instance();
    return execResult;
}
#endif
