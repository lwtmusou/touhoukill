#include "audio.h"
#include "mainwindow.h"
#include "server.h"
#include "settings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStyleFactory>
#include <QTranslator>

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "-server") == 0) {
        new QCoreApplication(argc, argv);
    } else {
        new QApplication(argc, argv);
        QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/plugins");

#ifndef Q_OS_WIN32
        if (QStyleFactory::keys().contains("Fusion", Qt::CaseInsensitive))
            qApp->setStyle(QStyleFactory::create("Fusion"));
#endif
    }

#ifndef Q_OS_ANDROID
#ifdef QT_NO_DEBUG
    QDir::setCurrent(QCoreApplication::instance()->applicationDirPath());
#endif
#else
    QDir::setCurrent("/sdcard/Android/data/rocks.touhousatsu.app");
#endif

#if defined(Q_OS_LINUX)
#ifndef Q_OS_ANDROID
    QDir::setCurrent(QCoreApplication::instance()->applicationFilePath().replace("games", "share"));
#else
    // extract data from assets
#endif
#endif

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    QCoreApplication::instance()->installTranslator(&qt_translator);
    QCoreApplication::instance()->installTranslator(&translator);

    Sanguosha = new Engine;
    Config.init();

    if (QCoreApplication::instance()->arguments().contains("-server")) {
        Server *server = new Server(QCoreApplication::instance());
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen())
            printf("Starting successfully\n");
        else
            printf("Starting failed!\n");

        int r = QCoreApplication::instance()->exec();
        delete qApp;
        return r;
    }

    QFile file("sanguosha.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        qApp->setStyleSheet(stream.readAll());
    }

    qApp->setFont(Config.AppFont);

#ifdef AUDIO_SUPPORT
    Audio::init();
#endif

    MainWindow *main_window = new MainWindow;

    Sanguosha->setParent(main_window);
    main_window->show();

    foreach (QString arg, QCoreApplication::instance()->arguments()) {
        if (arg.startsWith("-connect:")) {
            arg.remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostUrl", arg);

            main_window->startConnection();
            break;
        }
    }

    int execResult = QCoreApplication::instance()->exec();
    delete QCoreApplication::instance();
    return execResult;
}
