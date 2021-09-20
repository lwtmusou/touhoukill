#ifndef QSANGUOSHA_SERVERINFOWIDGET_H
#define QSANGUOSHA_SERVERINFOWIDGET_H

#include "QSanSelectableItem.h"
#include "player.h"
#include "protocol.h"

#include <QMap>
#include <QWidget>

class QLabel;
class QListWidget;
struct ServerInfoStruct;

class ServerInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ServerInfoWidget(bool show_lack = false);
    void fill(const ServerInfoStruct &info, const QString &address);
    void updateLack(int count);
    void clear();

private:
    QLabel *name_label;
    QLabel *address_label;
    QLabel *port_label;
    QLabel *game_mode_label;
    QLabel *player_count_label;
    QLabel *two_general_label;
    QLabel *same_label;
    QLabel *max_hp_label;
    QLabel *random_seat_label;
    QLabel *enable_cheat_label;
    QLabel *free_choose_label;
    QLabel *enable_ai_label;
    QLabel *time_limit_label;
    QLabel *lack_label;
    QListWidget *list_widget;
};
#endif
