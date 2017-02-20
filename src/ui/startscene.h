#ifndef _START_SCENE_H
#define _START_SCENE_H

#include "QSanSelectableItem.h"
#include "button.h"
#include "server.h"

#include <QAction>
#include <QGraphicsScene>
#include <QTextEdit>

class StartScene : public QGraphicsScene
{
    Q_OBJECT

public:
    StartScene();
    void addButton(QAction *action);
    void setServerLogBackground();
    void switchToServer(Server *server);

private:
    void printServerInfo();

    QSanSelectableItem *logo;
    QTextEdit *server_log;
    QList<Button *> buttons;
};

#endif
