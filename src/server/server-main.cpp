#include "engine.h"
#include "qsgsserver.h"
#include "serverconfig.h"
#include "serverinfostruct.h"

// Server is meant to be run in separate thread / process.

int startServer()
{
    ServerConfig.parse();
    return 0;
}

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
// mobile platforms don't use processes, so comment out main function
// WinRT is not supported, and won't be supported forever
#else
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Sanguosha->init();
    startServer();
    return a.exec();
}
#endif
