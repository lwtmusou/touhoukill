#include "cardoverview.h"
#include "SkinBank.h"
#include "audio.h"
#include "client.h"
#include "engine.h"
#include "package.h"
#include "roomscene.h"
#include "serverinfostruct.h"
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
        connect(ui->getCardButton, &QAbstractButton::clicked, this, &CardOverview::askCard);
    else
        ui->getCardButton->hide();

    ui->cardDescriptionBox->setProperty("description", true);
    ui->malePlayButton->hide();
    ui->femalePlayButton->hide();
    ui->playAudioEffectButton->hide();
}

void CardOverview::loadFromAll()
{
    int n = Sanguosha->cardCount();
    ui->tableWidget->setRowCount(n);
    for (int i = 0; i < n; i++)
        addCard(i, Sanguosha->cardDescriptor(i), i);

    if (n > 0) {
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));

        const CardDescriptor &card = Sanguosha->cardDescriptor(0);
        if (card.face()->type() == QSanguosha::TypeEquip) {
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

void CardOverview::loadFromList(const QList<int> &list)
{
    int n = list.length();
    ui->tableWidget->setRowCount(n);

    for (int i = 0; i < list.length(); ++i) {
        int id = list.value(i);
        addCard(i, Sanguosha->cardDescriptor(id), id);
    }

    if (n > 0) {
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));

        const CardDescriptor &card = Sanguosha->cardDescriptor(list.first());
        if (card.face()->type() == QSanguosha::TypeEquip) {
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

void CardOverview::loadFromList(const IdSet &list)
{
    loadFromList(list.values());
}

void CardOverview::addCard(int i, const CardDescriptor &card, int id)
{
    QString name = Sanguosha->translate(card.face()->name());
    QIcon suit_icon = QIcon(QStringLiteral("image/system/suit/%1.png").arg(Card::SuitToString(card.suit)));
    QString suit_str = Sanguosha->translate(Card::SuitToString(card.suit));
    QString point = Card::NumberToString(card.number);
    QString type = Sanguosha->translate(card.face()->name());
    QString subtype = Sanguosha->translate(card.face()->subTypeName());
    QString package = Sanguosha->translate(card.package->name());

    QTableWidgetItem *name_item = new QTableWidgetItem(name);
    name_item->setData(Qt::UserRole, id);

    ui->tableWidget->setItem(i, 0, name_item);
    ui->tableWidget->setItem(i, 1, new QTableWidgetItem(suit_icon, suit_str));
    ui->tableWidget->setItem(i, 2, new QTableWidgetItem(point));
    ui->tableWidget->setItem(i, 3, new QTableWidgetItem(type));
    ui->tableWidget->setItem(i, 4, new QTableWidgetItem(subtype));

    QTableWidgetItem *package_item = new QTableWidgetItem(package);
    if (Config.value(QStringLiteral("LuaPackages"), QString()).toString().split(QStringLiteral("+")).contains(card.package->name())) {
        package_item->setBackground(QBrush(qRgb(0x66, 0xCC, 0xFF)));
        package_item->setToolTip(tr("<font color=#FFFF33>This is an Lua extension</font>"));
    }

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
    const CardDescriptor &card = Sanguosha->cardDescriptor(card_id);
    QString pixmap_path = QStringLiteral("image/big-card/%1.png").arg(card.face()->name());
    ui->cardLabel->setPixmap(pixmap_path);

#if 0
    ui->cardDescriptionBox->setText(card.face()->description());
#endif

    if (card.face()->type() == QSanguosha::TypeEquip) {
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

void CardOverview::on_tableWidget_itemDoubleClicked(QTableWidgetItem * /*unused*/)
{
    if (Self != nullptr)
        askCard();
}

void CardOverview::on_malePlayButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const CardDescriptor &card = Sanguosha->cardDescriptor(card_id);
        Audio::playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card.face()->name(), true));
    }
}

void CardOverview::on_femalePlayButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const CardDescriptor &card = Sanguosha->cardDescriptor(card_id);
        Audio::playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card.face()->name(), false));
    }
}

void CardOverview::on_playAudioEffectButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
        const CardDescriptor &card = Sanguosha->cardDescriptor(card_id);
        if (card.face()->type() == QSanguosha::TypeEquip) {
#if 0
            QString effectName = card.face()->effectName();
            if (effectName == QStringLiteral("vscrossbow"))
                effectName = QStringLiteral("crossbow");
            QString fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(effectName, QStringLiteral("equip"), -1);
            if (!QFile::exists(fileName))
                fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(card.face()->commonEffectName(), QStringLiteral("common"), -1);
            Audio::playAudioEffect(fileName);
#endif
        }
    }
}
