#include "cardoverview.h"
#include "SkinBank.h"
#include "client.h"
#include "clientstruct.h"
#include "engine.h"
#include "roomscene.h"
#include "settings.h"
#include "ui_cardoverview.h"

#include <QFile>
#include <QMessageBox>

static CardOverview *Overview;

CardOverview *CardOverview::getInstance(QWidget *main_window)
{
    if (Overview == nullptr)
        Overview = new CardOverview(main_window);

    return Overview;
}

CardOverview::CardOverview(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CardOverview)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnWidth(0, 80);
    ui->tableWidget->setColumnWidth(1, 60);
    ui->tableWidget->setColumnWidth(2, 30);
    ui->tableWidget->setColumnWidth(3, 60);
    ui->tableWidget->setColumnWidth(4, 70);

    ui->tableWidget->setSortingEnabled(false);

    if (ServerInfo.EnableCheat)
        connect(ui->getCardButton, SIGNAL(clicked()), this, SLOT(askCard()));
    else
        ui->getCardButton->hide();

    ui->cardDescriptionBox->setProperty("description", true);
    ui->malePlayButton->hide();
    ui->femalePlayButton->hide();
    ui->playAudioEffectButton->hide();
}

void CardOverview::loadFromAll()
{
    int n = Sanguosha->getCardCount();
    ui->tableWidget->setRowCount(n);
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        addCard(i, card);
    }

    if (n > 0) {
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));

        const Card *card = Sanguosha->getEngineCard(0);
        if (card->face()->type() == CardFace::TypeEquip) {
            ui->playAudioEffectButton->show();
            ui->malePlayButton->hide();
            ui->femalePlayButton->hide();
        } else {
            ui->playAudioEffectButton->hide();
            ui->malePlayButton->show();
            ui->femalePlayButton->show();
        }
    }
}

void CardOverview::loadFromList(const QList<const Card *> &list)
{
    int n = list.length();
    ui->tableWidget->setRowCount(n);
    for (int i = 0; i < n; i++)
        addCard(i, list.at(i));

    if (n > 0) {
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));

        const Card *card = list.first();
        if (card->face()->type() == CardFace::TypeEquip) {
            ui->playAudioEffectButton->show();
            ui->malePlayButton->hide();
            ui->femalePlayButton->hide();
        } else {
            ui->playAudioEffectButton->hide();
            ui->malePlayButton->show();
            ui->femalePlayButton->show();
        }
    }
}

void CardOverview::addCard(int i, const Card *card)
{
    QString name = Sanguosha->translate(card->faceName());
    QIcon suit_icon = QIcon(QString("image/system/suit/%1.png").arg(card->suitString()));
    QString suit_str = Sanguosha->translate(card->suitString());
    QString point = card->numberString();
    QString type = Sanguosha->translate(card->face()->name());
    QString subtype = Sanguosha->translate(card->face()->subTypeName());
    QString package; //= Sanguosha->translate(card->getPackage());

    QTableWidgetItem *name_item = new QTableWidgetItem(name);
    name_item->setData(Qt::UserRole, card->id());

    ui->tableWidget->setItem(i, 0, name_item);
    ui->tableWidget->setItem(i, 1, new QTableWidgetItem(suit_icon, suit_str));
    ui->tableWidget->setItem(i, 2, new QTableWidgetItem(point));
    ui->tableWidget->setItem(i, 3, new QTableWidgetItem(type));
    ui->tableWidget->setItem(i, 4, new QTableWidgetItem(subtype));

    QTableWidgetItem *package_item = new QTableWidgetItem(package);
#if 0
    if (Config.value("LuaPackages", QString()).toString().split("+").contains(card->getPackage())) {
        package_item->setBackground(QBrush(qRgb(0x66, 0xCC, 0xFF)));
        package_item->setToolTip(tr("<font color=#FFFF33>This is an Lua extension</font>"));
    }
#endif
    ui->tableWidget->setItem(i, 5, package_item);
}

CardOverview::~CardOverview()
{
    delete ui;
}

void CardOverview::on_tableWidget_itemSelectionChanged()
{
    int row = ui->tableWidget->currentRow();
    int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
    const Card *card = Sanguosha->getEngineCard(card_id);
    QString pixmap_path = QString("image/big-card/%1.png").arg(card->faceName());
    ui->cardLabel->setPixmap(pixmap_path);

    ui->cardDescriptionBox->setText(card->face()->description());

    if (card->face()->type() == CardFace::TypeEquip) {
        ui->playAudioEffectButton->show();
        ui->malePlayButton->hide();
        ui->femalePlayButton->hide();
    } else {
        ui->playAudioEffectButton->hide();
        ui->malePlayButton->show();
        ui->femalePlayButton->show();
    }
}

void CardOverview::askCard()
{
    if (!ServerInfo.EnableCheat)
        return;

    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        if (!ClientInstance->getAvailableCards().contains(card_id)) {
            QMessageBox::warning(this, tr("Warning"), tr("These packages don't contain this card"));
            return;
        }
        ClientInstance->requestCheatGetOneCard(card_id);
    }
}

void CardOverview::on_tableWidget_itemDoubleClicked(QTableWidgetItem *)
{
    if (Self)
        askCard();
}

void CardOverview::on_malePlayButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const Card *card = Sanguosha->getEngineCard(card_id);
        Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card->faceName(), true));
    }
}

void CardOverview::on_femalePlayButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const Card *card = Sanguosha->getEngineCard(card_id);
        Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card->faceName(), false));
    }
}

void CardOverview::on_playAudioEffectButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const Card *card = Sanguosha->getEngineCard(card_id);
        if (card->face()->type() == CardFace::TypeEquip) {
            QString effectName = card->face()->effectName();
            if (effectName == "vscrossbow")
                effectName = "crossbow";
            QString fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(effectName, QString("equip"), -1);
            if (!QFile::exists(fileName))
                fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(card->face()->commonEffectName(), QString("common"), -1);
            Sanguosha->playAudioEffect(fileName);
        }
    }
}
