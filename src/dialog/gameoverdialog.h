#ifndef THKILL_GAME_OVER_DIALOG_H
#define THKILL_GAME_OVER_DIALOG_H

#include <QDialog>

class ClientPlayer;
class QTableWidget;
class QVBoxLayout;

class GameOverDialog : public QDialog
{
    Q_OBJECT

public:
    // standoff == true: single table of all players (standoff result).
    // standoff == false: split winner/loser tables by each player's "win" property.
    explicit GameOverDialog(bool standoff, QWidget *parent);

private:
    void fillTable(QTableWidget *table, const QList<ClientPlayer *> &players);
    void addReturnButton(QVBoxLayout *layout);
};

#endif
