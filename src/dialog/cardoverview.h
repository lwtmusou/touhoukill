#ifndef _CARD_OVERVIEW_H
#define _CARD_OVERVIEW_H

#include "card.h"

#include <QDialog>
#include <QTableWidgetItem>

class MainWindow;
namespace Ui {
class CardOverview;
}

class CardOverview : public QDialog
{
    Q_OBJECT

public:
    static CardOverview *getInstance(QWidget *main_window);

    explicit CardOverview(QWidget *parent = nullptr);
    void loadFromAll();
    void loadFromList(const QList<int> &list);
    void loadFromList(const IDSet &list);

    ~CardOverview() override;

private:
    Ui::CardOverview *ui;

    void addCard(int i, const CardDescriptor &card, int id);

private slots:
    void on_femalePlayButton_clicked();
    void on_malePlayButton_clicked();
    void on_playAudioEffectButton_clicked();
    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);
    void on_tableWidget_itemSelectionChanged();
    void askCard();
};

#endif
