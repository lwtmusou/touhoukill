#include "server.h"
#include "serverinfostruct.h"

// Server is meant to be run in separate thread / process.

Server::Server()
{
    if (!ServerInfo.parsed()) {
        // initialize ServerInfo from parsed arguments & settings
    }

    // start a listening server
    // TODO
}

int startServer(int argc, char *argv[])
{
}

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID)
// mobile platforms don't use processes, so comment out main function
// WinRT is not supported, and won't be supported forever
#else
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // TODO
    return a.exec();
}
#endif
