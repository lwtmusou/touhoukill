#ifndef TOUHOUKILL_SERVER_H_
#define TOUHOUKILL_SERVER_H_

#include <QObject>

class Server : public QObject
{
    Q_OBJECT

public:
    Server();
    ~Server() override = default;
};

#endif
