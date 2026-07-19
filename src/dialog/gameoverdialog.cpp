#include "gameoverdialog.h"

#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "mainwindow.h"
#include "record-analysis.h"
#include "settings.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

GameOverDialog::GameOverDialog(bool standoff, QWidget *parent)
    : QDialog(parent)
{
    QList<ClientPlayer *> players = ClientInstance->getPlayers();

    auto *layout = new QVBoxLayout;

    if (standoff) {
        setWindowTitle(tr("Standoff"));
        resize(500, 600);

        auto *table = new QTableWidget;
        fillTable(table, players);
        layout->addWidget(table);
    } else {
        bool victory = Self->property("win").toBool();
        setWindowTitle(victory ? tr("Victory") : tr("Failure"));
        resize(800, 600);

        QList<ClientPlayer *> winner_list;
        QList<ClientPlayer *> loser_list;
        for (ClientPlayer *player : players) {
            if (player->property("win").toBool())
                winner_list << player;
            else
                loser_list << player;
        }

        auto *winner_box = new QGroupBox(tr("Winner(s)"));
        auto *winner_table = new QTableWidget;
        auto *winner_layout = new QVBoxLayout;
        winner_layout->addWidget(winner_table);
        winner_box->setLayout(winner_layout);

        auto *loser_box = new QGroupBox(tr("Loser(s)"));
        auto *loser_table = new QTableWidget;
        auto *loser_layout = new QVBoxLayout;
        loser_layout->addWidget(loser_table);
        loser_box->setLayout(loser_layout);

        fillTable(winner_table, winner_list);
        fillTable(loser_table, loser_list);

        layout->addWidget(winner_box);
        layout->addWidget(loser_box);
    }

    setLayout(layout);
    addReturnButton(layout);
}

void GameOverDialog::fillTable(QTableWidget *table, const QList<ClientPlayer *> &players)
{
    table->setColumnCount(10);
    table->setRowCount(static_cast<int>(players.length()));
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    RecAnalysis record(ClientInstance->getReplayPath());
    QMap<QString, PlayerRecordStruct *> record_map = record.getRecordMap();

    static QStringList labels;
    if (labels.isEmpty()) {
        labels << tr("General") << tr("Name") << tr("Alive");
        labels << tr("Role");
        labels << tr("TurnCount");
        labels << tr("Recover") << tr("Damage") << tr("Damaged") << tr("Kill");
        labels << tr("Handcards");
    }
    table->setHorizontalHeaderLabels(labels);
    table->setSelectionBehavior(QTableWidget::SelectRows);

    for (int i = 0; i < players.length(); i++) {
        ClientPlayer *player = players[i];

        auto *item = new QTableWidgetItem;
        item->setText(ClientInstance->getPlayerName(player->objectName()));
        table->setItem(i, 0, item);

        item = new QTableWidgetItem;
        item->setText(player->screenName());
        table->setItem(i, 1, item);

        item = new QTableWidgetItem;
        item->setText(player->isAlive() ? tr("Alive") : tr("Dead"));
        table->setItem(i, 2, item);

        item = new QTableWidgetItem;
        QIcon icon(QString("image/system/roles/%1.png").arg(player->getRole()));
        item->setIcon(icon);
        QString role = player->getRole();
        if (ServerInfo.GameMode.startsWith("06_")) {
            if (role == "lord" || role == "renegade")
                role = "leader";
            else
                role = "guard";
        } else if (ServerInfo.GameMode == "04_1v3") {
            int seat = player->getSeat();
            switch (seat) {
            case 1: role = "lvbu"; break;
            case 2: role = "vanguard"; break;
            case 3: role = "mainstay"; break;
            case 4: role = "general"; break;
            default: break;
            }
        } else if (ServerInfo.GameMode == "02_1v1") {
            if (role == "lord")
                role = "defensive";
            else
                role = "offensive";
        }
        item->setText(Sanguosha->translate(role));

        if (!player->isAlive())
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        table->setItem(i, 3, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(player->getMark("Global_TurnCount")));
        table->setItem(i, 4, item);

        PlayerRecordStruct *rec = record_map.value(player->objectName());
        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_recover));
        table->setItem(i, 5, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damage));
        table->setItem(i, 6, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damaged));
        table->setItem(i, 7, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_kill));
        table->setItem(i, 8, item);

        item = new QTableWidgetItem;
        QString handcards = QString::fromUtf8(QByteArray::fromBase64(player->property("last_handcards").toString().toLatin1()));
        handcards.replace("<img src='image/system/log/spade.png' height = 12/>", tr("Spade"));
        handcards.replace("<img src='image/system/log/heart.png' height = 12/>", tr("Heart"));
        handcards.replace("<img src='image/system/log/club.png' height = 12/>", tr("Club"));
        handcards.replace("<img src='image/system/log/diamond.png' height = 12/>", tr("Diamond"));
        item->setText(handcards);
        table->setItem(i, 9, item);
    }

    for (int i = 2; i <= 10; i++)
        table->resizeColumnToContents(i);
}

void GameOverDialog::addReturnButton(QVBoxLayout *layout)
{
    resize(MainWindowInstance->width() / 2, height());

    auto *hlayout = new QHBoxLayout;
    hlayout->addStretch();

    // TODO: re-add "Restart Game" and "Save record" buttons once restart signal and
    // replay-record saving are wired into the QML bridge. Old code had them via
    // RoomScene::restart() / saveReplayRecord(), currently commented out in mainwindow.cpp.

    auto *return_button = new QPushButton(tr("Return to main menu"));
    connect(return_button, &QPushButton::clicked, this, [this]() {
        accept();
        // Defer the scene switch to the next event-loop iteration so this dialog's
        // exec() stack frame unwinds before MainWindow::gotoStartScene destroys the
        // RoomScene that owns the bridge call showing this dialog.
        QTimer::singleShot(0, MainWindowInstance, &MainWindow::gotoStartScene);
    });
    hlayout->addWidget(return_button);

    layout->addLayout(hlayout);
}
