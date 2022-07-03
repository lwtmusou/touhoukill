#include "serverinfowidget.h"
#include "client.h"
#include "engine.h"
#include "jsonutils.h"
#include "package.h"
#include "settings.h"
#include "structs.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QListWidget>

ServerInfoWidget::ServerInfoWidget(bool show_lack)
{
    name_label = new QLabel;
    address_label = new QLabel;
    port_label = new QLabel;
    game_mode_label = new QLabel;
    player_count_label = new QLabel;
    two_general_label = new QLabel;
    same_label = new QLabel;
    random_seat_label = new QLabel;
    enable_cheat_label = new QLabel;
    free_choose_label = new QLabel;
    enable_ai_label = new QLabel;
    time_limit_label = new QLabel;
    max_hp_label = new QLabel;

    list_widget = new QListWidget;
    list_widget->setViewMode(QListView::IconMode);
    list_widget->setMovement(QListView::Static);

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Server name"), name_label);
    layout->addRow(tr("Address"), address_label);
    layout->addRow(tr("Port"), port_label);
    layout->addRow(tr("Game mode"), game_mode_label);
    layout->addRow(tr("Player count"), player_count_label);
    layout->addRow(tr("2nd general mode"), two_general_label);
    layout->addRow(tr("Same Mode"), same_label);
    layout->addRow(tr("Max HP scheme"), max_hp_label);
    layout->addRow(tr("Random seat"), random_seat_label);
    layout->addRow(tr("Enable cheat"), enable_cheat_label);
    layout->addRow(tr("Free choose"), free_choose_label);
    layout->addRow(tr("Enable AI"), enable_ai_label);
    layout->addRow(tr("Operation time"), time_limit_label);
    layout->addRow(tr("Extension packages"), list_widget);

    if (show_lack) {
        lack_label = new QLabel;
        layout->addRow(tr("Lack"), lack_label);
    } else
        lack_label = nullptr;

    setLayout(layout);
}

void ServerInfoWidget::fill(const ServerInfoStruct &info, const QString &address)
{
    name_label->setText(info.Name);
    address_label->setText(address);
    game_mode_label->setText(Sanguosha->translate(info.GameModeStr));
    int player_count = Sanguosha->getPlayerCount(info.GameModeStr);
    player_count_label->setText(QString::number(player_count));
    port_label->setText(QString::number(Config.ServerPort));
    two_general_label->setText(info.isMultiGeneralEnabled() ? tr("Enabled") : tr("Disabled"));
    same_label->setText(tr("Disabled"));

    if (info.isMultiGeneralEnabled()) {
        max_hp_label->setText(QString(tr("Sum - %1")).arg(3));
    } else {
        max_hp_label->setText(tr("2nd general is disabled"));
        max_hp_label->setEnabled(false);
    }

    random_seat_label->setText(info.RandomSeat ? tr("Enabled") : tr("Disabled"));
    enable_cheat_label->setText(info.EnableCheat ? tr("Enabled") : tr("Disabled"));
    free_choose_label->setText(info.FreeChoose ? tr("Enabled") : tr("Disabled"));
    enable_ai_label->setText(info.EnableAI ? tr("Enabled") : tr("Disabled"));

    if (info.OperationTimeout == 0)
        time_limit_label->setText(tr("No limit"));
    else
        time_limit_label->setText(tr("%1 seconds").arg(info.OperationTimeout));

    list_widget->clear();

    static QIcon enabled_icon(QStringLiteral("image/system/enabled.png"));
    static QIcon disabled_icon(QStringLiteral("image/system/disabled.png"));

    foreach (QString extension, info.EnabledPackages) {
        bool checked = !extension.startsWith(QStringLiteral("!"));
        if (!checked)
            extension.remove(QStringLiteral("!"));

        QString package_name = Sanguosha->translate(extension);
        QCheckBox *checkbox = new QCheckBox(package_name);
        checkbox->setChecked(checked);

        new QListWidgetItem(checked ? enabled_icon : disabled_icon, package_name, list_widget);
    }
}

void ServerInfoWidget::updateLack(int count)
{
    if (lack_label != nullptr) {
        QString path = QStringLiteral("image/system/number/%1.png").arg(count);
        lack_label->setPixmap(QPixmap(path));
    }
}

void ServerInfoWidget::clear()
{
    name_label->clear();
    address_label->clear();
    port_label->clear();
    game_mode_label->clear();
    player_count_label->clear();
    two_general_label->clear();
    same_label->clear();
    random_seat_label->clear();
    enable_cheat_label->clear();
    free_choose_label->clear();
    time_limit_label->clear();
    list_widget->clear();
}
