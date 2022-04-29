#include "server.h"
#include "SkinBank.h"
#include "choosegeneraldialog.h"
#include "engine.h"
#include "general.h"
#include "nativesocket.h"
#include "package.h"
#include "protocol.h"
#include "room.h"
#include "settings.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

using namespace QSanProtocol;

static QLayout *HLay(QWidget *left, QWidget *right)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(left);
    layout->addWidget(right);
    return layout;
}

ServerDialog::ServerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Start server"));

    QTabWidget *tab_widget = new QTabWidget;
    tab_widget->addTab(createBasicTab(), tr("Basic"));
    tab_widget->addTab(createPackageTab(), tr("Game Pacakge Selection"));
    tab_widget->addTab(createAdvancedTab(), tr("Advanced"));
    tab_widget->addTab(createMiscTab(), tr("Miscellaneous"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(tab_widget);
    layout->addLayout(createButtonLayout());
    setLayout(layout);

    setMinimumWidth(300);
}

QWidget *ServerDialog::createBasicTab()
{
    server_name_edit = new QLineEdit;
    server_name_edit->setText(Config.ServerName);

    timeout_spinbox = new QSpinBox;
    timeout_spinbox->setMinimum(5);
    timeout_spinbox->setMaximum(60);
    timeout_spinbox->setValue(Config.OperationTimeout);
    timeout_spinbox->setSuffix(tr(" seconds"));
    nolimit_checkbox = new QCheckBox(tr("No limit"));
    nolimit_checkbox->setChecked(Config.OperationNoLimit);
    connect(nolimit_checkbox, &QAbstractButton::toggled, timeout_spinbox, &QWidget::setDisabled);

    // add 1v1 banlist edit button
    QPushButton *edit_button = new QPushButton(tr("Banlist ..."));
    edit_button->setFixedWidth(100);
    connect(edit_button, &QAbstractButton::clicked, this, &ServerDialog::edit1v1Banlist);

    QFormLayout *form_layout = new QFormLayout;
    form_layout->addRow(tr("Server name"), server_name_edit);
    QHBoxLayout *lay = new QHBoxLayout;
    lay->addWidget(timeout_spinbox);
    lay->addWidget(nolimit_checkbox);
    lay->addWidget(edit_button);
    form_layout->addRow(tr("Operation timeout"), lay);
    form_layout->addRow(createGameModeBox());

    QWidget *widget = new QWidget;
    widget->setLayout(form_layout);
    return widget;
}

QWidget *ServerDialog::createPackageTab()
{
    extension_group = new QButtonGroup;
    extension_group->setExclusive(false);

    QStringList extensions = Sanguosha->packangeNames();
    QSet<QString> ban_packages(Config.BanPackages.begin(), Config.BanPackages.end());

    QGroupBox *box1 = new QGroupBox(tr("General package"));
    QGroupBox *box2 = new QGroupBox(tr("Card package"));

    QGridLayout *layout1 = new QGridLayout;
    QGridLayout *layout2 = new QGridLayout;
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    int i = 0;
    int j = 0;
    int row = 0;
    int column = 0;
    foreach (QString extension, extensions) {
        const Package *package = Sanguosha->findPackage(extension);
        if (package == nullptr)
            continue;

        bool forbid_package = Config.value(QStringLiteral("ForbidPackages")).toStringList().contains(extension);
        QCheckBox *checkbox = new QCheckBox;
        checkbox->setObjectName(extension);
        checkbox->setText(Sanguosha->translate(extension));
        checkbox->setChecked(!ban_packages.contains(extension) && !forbid_package);
        checkbox->setEnabled(!forbid_package);

        extension_group->addButton(checkbox);

        switch (package->type()) {
        case QSanguosha::GeneralPack: {
            if (extension == QStringLiteral("standard") || extension == QStringLiteral("test"))
                continue;
            row = i / 5;
            column = i % 5;
            i++;

            layout1->addWidget(checkbox, row, column + 1);
            break;
        }
        case QSanguosha::CardPack: {
            row = j / 5;
            column = j % 5;
            j++;

            layout2->addWidget(checkbox, row, column + 1);
            break;
        }
        default:
            break;
        }
    }

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(box1);
    layout->addWidget(box2);

    widget->setLayout(layout);
    return widget;
}

QWidget *ServerDialog::createAdvancedTab()
{
    QVBoxLayout *layout = new QVBoxLayout;

    random_seat_checkbox = new QCheckBox(tr("Arrange the seats randomly"));
    random_seat_checkbox->setChecked(Config.RandomSeat);

    assign_latest_general_checkbox = new QCheckBox(tr("Assign Latest General"));
    assign_latest_general_checkbox->setChecked(Config.AssignLatestGeneral);

    enable_cheat_checkbox = new QCheckBox(tr("Enable cheat"));
    enable_cheat_checkbox->setToolTip(tr("<font color=#FFFF33>This option enables the cheat menu</font>"));
    enable_cheat_checkbox->setChecked(Config.EnableCheat);

    free_choose_checkbox = new QCheckBox(tr("Choose generals and cards freely"));
    free_choose_checkbox->setChecked(Config.FreeChoose);
    free_choose_checkbox->setVisible(Config.EnableCheat);

    free_assign_checkbox = new QCheckBox(tr("Assign role and seat freely"));
    free_assign_checkbox->setChecked(Config.value(QStringLiteral("FreeAssign")).toBool());
    free_assign_checkbox->setVisible(Config.EnableCheat);

    free_assign_self_checkbox = new QCheckBox(tr("Assign only your own role"));
    free_assign_self_checkbox->setChecked(Config.FreeAssignSelf);
    free_assign_self_checkbox->setEnabled(free_assign_checkbox->isChecked());
    free_assign_self_checkbox->setVisible(Config.EnableCheat);

    connect(enable_cheat_checkbox, &QAbstractButton::toggled, free_choose_checkbox, &QWidget::setVisible);
    connect(enable_cheat_checkbox, &QAbstractButton::toggled, free_assign_checkbox, &QWidget::setVisible);
    connect(enable_cheat_checkbox, &QAbstractButton::toggled, free_assign_self_checkbox, &QWidget::setVisible);
    connect(free_assign_checkbox, &QAbstractButton::toggled, free_assign_self_checkbox, &QWidget::setEnabled);

    pile_swapping_label = new QLabel(tr("Pile-swapping limitation"));
    pile_swapping_label->setToolTip(tr("<font color=#FFFF33>-1 means no limitations</font>"));
    pile_swapping_spinbox = new QSpinBox;
    pile_swapping_spinbox->setRange(-1, 15);
    pile_swapping_spinbox->setValue(Config.value(QStringLiteral("PileSwappingLimitation"), 5).toInt());

    without_lordskill_checkbox = new QCheckBox(tr("Without Lordskill"));
    without_lordskill_checkbox->setChecked(Config.value(QStringLiteral("WithoutLordskill"), false).toBool());

    sp_convert_checkbox = new QCheckBox(tr("Enable SP Convert"));
    sp_convert_checkbox->setChecked(Config.value(QStringLiteral("EnableSPConvert"), true).toBool());

    maxchoice_spinbox = new QSpinBox;
    maxchoice_spinbox->setRange(3, 10);
    maxchoice_spinbox->setValue(Config.value(QStringLiteral("MaxChoice"), 6).toInt());

    godlimit_label = new QLabel(tr("Upperlimit for gods"));
    godlimit_label->setToolTip(tr("<font color=#FFFF33>-1 means that all gods may appear in your general chosen dialog!</font>"));
    godlimit_spinbox = new QSpinBox;
    godlimit_spinbox->setRange(-1, 8);
    godlimit_spinbox->setValue(Config.value(QStringLiteral("GodLimit"), 1).toInt());

    lord_maxchoice_label = new QLabel(tr("Upperlimit for lord"));
    lord_maxchoice_label->setToolTip(tr("<font color=#FFFF33>-1 means that all lords are available</font>"));
    lord_maxchoice_spinbox = new QSpinBox;
    lord_maxchoice_spinbox->setRange(-1, 10);
    lord_maxchoice_spinbox->setValue(Config.value(QStringLiteral("LordMaxChoice"), 6).toInt());

    nonlord_maxchoice_spinbox = new QSpinBox;
    nonlord_maxchoice_spinbox->setRange(0, 10);
    nonlord_maxchoice_spinbox->setValue(Config.value(QStringLiteral("NonLordMaxChoice"), 6).toInt());

    forbid_same_ip_checkbox = new QCheckBox(tr("Forbid same IP with multiple connection"));
    forbid_same_ip_checkbox->setChecked(Config.ForbidSIMC);

    disable_chat_checkbox = new QCheckBox(tr("Disable chat"));
    disable_chat_checkbox->setChecked(Config.DisableChat);

    second_general_checkbox = new QCheckBox(tr("Enable second general"));
    second_general_checkbox->setChecked(Config.Enable2ndGeneral);

    prevent_awaken_below3_checkbox = new QCheckBox(tr("Prevent maxhp being less than 3 for awaken skills"));
    prevent_awaken_below3_checkbox->setChecked(Config.PreventAwakenBelow3);
    prevent_awaken_below3_checkbox->setEnabled(false);

    address_edit = new QLineEdit;
    address_edit->setText(Config.Address);
    address_edit->setPlaceholderText(tr("Public IP or domain"));

    QPushButton *detect_button = new QPushButton(tr("Detect my WAN IP"));
    connect(detect_button, &QAbstractButton::clicked, this, &ServerDialog::onDetectButtonClicked);

    port_edit = new QLineEdit;
    port_edit->setText(QString::number(Config.ServerPort));
    port_edit->setValidator(new QIntValidator(1, 9999, port_edit));

    layout->addWidget(forbid_same_ip_checkbox);
    layout->addWidget(disable_chat_checkbox);
    layout->addLayout(HLay(random_seat_checkbox, assign_latest_general_checkbox));
    layout->addWidget(enable_cheat_checkbox);
    layout->addWidget(free_choose_checkbox);
    layout->addLayout(HLay(free_assign_checkbox, free_assign_self_checkbox));
    layout->addLayout(HLay(pile_swapping_label, pile_swapping_spinbox));
    layout->addLayout(HLay(without_lordskill_checkbox, sp_convert_checkbox));
    layout->addLayout(HLay(new QLabel(tr("Upperlimit for general")), maxchoice_spinbox));
    layout->addLayout(HLay(godlimit_label, godlimit_spinbox));
    layout->addLayout(HLay(lord_maxchoice_label, lord_maxchoice_spinbox));
    layout->addLayout(HLay(new QLabel(tr("Upperlimit for non-lord")), nonlord_maxchoice_spinbox));
    layout->addWidget(second_general_checkbox);
    layout->addWidget(prevent_awaken_below3_checkbox);
    layout->addLayout(HLay(new QLabel(tr("Address")), address_edit));
    layout->addWidget(detect_button);
    layout->addLayout(HLay(new QLabel(tr("Port")), port_edit));
    layout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    if (!Config.Enable2ndGeneral)
        prevent_awaken_below3_checkbox->setVisible(false);

    return widget;
}

QWidget *ServerDialog::createMiscTab()
{
    game_start_spinbox = new QSpinBox;
    game_start_spinbox->setRange(0, 10);
    game_start_spinbox->setValue(Config.CountDownSeconds);
    game_start_spinbox->setSuffix(tr(" seconds"));

    nullification_spinbox = new QSpinBox;
    nullification_spinbox->setRange(5, 15);
    nullification_spinbox->setValue(Config.NullificationCountDown);
    nullification_spinbox->setSuffix(tr(" seconds"));

    minimize_dialog_checkbox = new QCheckBox(tr("Minimize the dialog when server runs"));
    minimize_dialog_checkbox->setChecked(Config.EnableMinimizeDialog);

    surrender_at_death_checkbox = new QCheckBox(tr("Surrender at the time of Death"));
    surrender_at_death_checkbox->setChecked(Config.SurrenderAtDeath);

    luck_card_label = new QLabel(tr("Upperlimit for use time of luck card"));
    luck_card_spinbox = new QSpinBox;
    luck_card_spinbox->setRange(0, 3);
    luck_card_spinbox->setValue(Config.LuckCardLimitation);

    QGroupBox *ai_groupbox = new QGroupBox(tr("Artificial intelligence"));
    ai_groupbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *layout = new QVBoxLayout;

    ai_enable_checkbox = new QCheckBox(tr("Enable AI"));
    ai_enable_checkbox->setChecked(Config.EnableAI);

    ai_delay_spinbox = new QSpinBox;
    ai_delay_spinbox->setMinimum(0);
    ai_delay_spinbox->setMaximum(5000);
    ai_delay_spinbox->setValue(Config.OriginAIDelay);
    ai_delay_spinbox->setSuffix(tr(" millisecond"));

    ai_delay_altered_checkbox = new QCheckBox(tr("Alter AI Delay After Death"));
    ai_delay_altered_checkbox->setChecked(Config.AlterAIDelayAD);

    ai_delay_ad_spinbox = new QSpinBox;
    ai_delay_ad_spinbox->setMinimum(0);
    ai_delay_ad_spinbox->setMaximum(5000);
    ai_delay_ad_spinbox->setValue(Config.AIDelayAD);
    ai_delay_ad_spinbox->setSuffix(tr(" millisecond"));
    ai_delay_ad_spinbox->setEnabled(ai_delay_altered_checkbox->isChecked());
    connect(ai_delay_altered_checkbox, &QAbstractButton::toggled, ai_delay_ad_spinbox, &QWidget::setEnabled);

    ai_prohibit_blind_attack_checkbox = new QCheckBox(tr("Prohibit Blind Attack"));
    ai_prohibit_blind_attack_checkbox->setToolTip(tr("<font color=#FFFF33>ai will not blindly attack,if this is checked</font>"));
    ai_prohibit_blind_attack_checkbox->setChecked(Config.AIProhibitBlindAttack);

    limit_robot_checkbox = new QCheckBox(tr("Limit AI"));
    limit_robot_checkbox->setToolTip(tr("<font color=#FFFF33>Prohibit add AI when the player num is less than 4 and there is no more than a half of them are human</font>"));
    limit_robot_checkbox->setChecked(Config.LimitRobot);

    layout->addWidget(ai_enable_checkbox);
    layout->addLayout(HLay(new QLabel(tr("AI delay")), ai_delay_spinbox));
    layout->addWidget(ai_delay_altered_checkbox);
    layout->addLayout(HLay(new QLabel(tr("AI delay After Death")), ai_delay_ad_spinbox));
    layout->addWidget(ai_prohibit_blind_attack_checkbox);
    layout->addWidget(limit_robot_checkbox);

    ai_groupbox->setLayout(layout);

    QVBoxLayout *tablayout = new QVBoxLayout;
    tablayout->addLayout(HLay(new QLabel(tr("Game start count down")), game_start_spinbox));
    tablayout->addLayout(HLay(new QLabel(tr("Nullification count down")), nullification_spinbox));
    tablayout->addWidget(minimize_dialog_checkbox);
    tablayout->addWidget(surrender_at_death_checkbox);
    tablayout->addLayout(HLay(luck_card_label, luck_card_spinbox));
    tablayout->addWidget(luck_card_spinbox);
    tablayout->addWidget(ai_groupbox);
    tablayout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(tablayout);
    return widget;
}

void ServerDialog::updateButtonEnablility(QAbstractButton *button)
{
}

void BanlistDialog::switchTo(int item)
{
    this->item = item;
    list = lists.at(item);
}

BanlistDialog::BanlistDialog(QWidget *parent, bool view)
    : QDialog(parent)
{
    setWindowTitle(tr("Select generals that are excluded"));

    if (ban_list.isEmpty())
        ban_list << QStringLiteral("Roles") << QStringLiteral("HulaoPass");
    QVBoxLayout *layout = new QVBoxLayout;

    QTabWidget *tab = new QTabWidget;
    layout->addWidget(tab);
    connect(tab, &QTabWidget::currentChanged, this, &BanlistDialog::switchTo);

    foreach (const QString &item, ban_list) {
        QWidget *apage = new QWidget;

        list = new QListWidget;
        list->setObjectName(item);

        QStringList banlist = Config.value(QStringLiteral("Banlist/%1").arg(item)).toStringList();
        foreach (QString name, banlist)
            addGeneral(name);

        lists << list;

        QVBoxLayout *vlay = new QVBoxLayout;
        vlay->addWidget(list);
        apage->setLayout(vlay);

        tab->addTab(apage, Sanguosha->translate(item));
    }

    QPushButton *add = new QPushButton(tr("Add ..."));
    QPushButton *remove = new QPushButton(tr("Remove"));
    QPushButton *ok = new QPushButton(tr("OK"));

    connect(ok, &QAbstractButton::clicked, this, &QDialog::accept);
    connect(this, &QDialog::accepted, this, &BanlistDialog::saveAll);
    connect(remove, &QAbstractButton::clicked, this, &BanlistDialog::doRemoveButton);
    connect(add, &QAbstractButton::clicked, this, &BanlistDialog::doAddButton);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    if (!view) {
        hlayout->addWidget(add);
        hlayout->addWidget(remove);
        list = lists.first();
    }

    hlayout->addWidget(ok);
    layout->addLayout(hlayout);

    setLayout(layout);

    foreach (QListWidget *alist, lists) {
        alist->setViewMode(QListView::IconMode);
        alist->setDragDropMode(QListView::NoDragDrop);
    }
}

void BanlistDialog::addGeneral(const QString &name)
{
    QIcon icon(G_ROOM_SKIN.getGeneralPixmap(name, QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY, false));
    QString text = Sanguosha->translate(name);
    QListWidgetItem *item = new QListWidgetItem(icon, text, list);
    item->setSizeHint(QSize(60, 60));
    item->setData(Qt::UserRole, name);
}

void BanlistDialog::doAddButton()
{
    FreeChooseDialog *chooser = new FreeChooseDialog(this, false);
    connect(chooser, &FreeChooseDialog::general_chosen, this, &BanlistDialog::addGeneral);
    chooser->exec();
}

void BanlistDialog::doRemoveButton()
{
    int row = list->currentRow();
    if (row != -1)
        delete list->takeItem(row);
}

void BanlistDialog::save()
{
    QSet<QString> banset;

    for (int i = 0; i < list->count(); i++)
        banset << list->item(i)->data(Qt::UserRole).toString();

    QStringList banlist = banset.values();
    Config.setValue(QStringLiteral("Banlist/%1").arg(ban_list.at(item)), QVariant::fromValue(banlist));
}

void BanlistDialog::saveAll()
{
    for (int i = 0; i < lists.length(); i++) {
        switchTo(i);
        save();
    }
}

void ServerDialog::edit1v1Banlist()
{
    BanlistDialog *dialog = new BanlistDialog(this);
    dialog->exec();
}

QGroupBox *ServerDialog::create1v1Box()
{
    QGroupBox *box = new QGroupBox(tr("1v1 options"));
    box->setEnabled(Config.GameMode == QStringLiteral("02_1v1"));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout;

    QComboBox *officialComboBox = new QComboBox;
    officialComboBox->addItem(tr("Classical"), QStringLiteral("Classical"));
    officialComboBox->addItem(QStringLiteral("2013"), QStringLiteral("2013"));
    officialComboBox->addItem(QStringLiteral("OL"), QStringLiteral("OL"));

    official_1v1_ComboBox = officialComboBox;

    QString rule = Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString();
    if (rule == QStringLiteral("2013"))
        officialComboBox->setCurrentIndex(1);
    else if (rule == QStringLiteral("OL"))
        officialComboBox->setCurrentIndex(2);

    vlayout->addLayout(HLay(new QLabel(tr("Rule option")), official_1v1_ComboBox));

    box->setLayout(vlayout);

    return box;
}

QGroupBox *ServerDialog::create3v3Box()
{
    QGroupBox *box = new QGroupBox(tr("3v3 options"));
    box->setEnabled(Config.GameMode == QStringLiteral("06_3v3"));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout;

    QRadioButton *extend = new QRadioButton(tr("Extension mode"));
    QPushButton *extend_edit_button = new QPushButton(tr("General selection ..."));
    extend_edit_button->setEnabled(false);
    connect(extend, &QAbstractButton::toggled, extend_edit_button, &QWidget::setEnabled);
    connect(extend_edit_button, &QAbstractButton::clicked, this, &ServerDialog::select3v3Generals);

    exclude_disaster_checkbox = new QCheckBox(tr("Exclude disasters"));
    exclude_disaster_checkbox->setChecked(Config.value(QStringLiteral("3v3/ExcludeDisasters"), true).toBool());

    QComboBox *roleChooseComboBox = new QComboBox;
    roleChooseComboBox->addItem(tr("Normal"), QStringLiteral("Normal"));
    roleChooseComboBox->addItem(tr("Random"), QStringLiteral("Random"));
    roleChooseComboBox->addItem(tr("All roles"), QStringLiteral("AllRoles"));

    role_choose_ComboBox = roleChooseComboBox;

    QString scheme = Config.value(QStringLiteral("3v3/RoleChoose"), QStringLiteral("Normal")).toString();
    if (scheme == QStringLiteral("Random"))
        roleChooseComboBox->setCurrentIndex(1);
    else if (scheme == QStringLiteral("AllRoles"))
        roleChooseComboBox->setCurrentIndex(2);

    vlayout->addLayout(HLay(extend, extend_edit_button));
    vlayout->addWidget(exclude_disaster_checkbox);
    vlayout->addLayout(HLay(new QLabel(tr("Role choose")), role_choose_ComboBox));
    box->setLayout(vlayout);

    extend->setChecked(true);

    return box;
}

QGroupBox *ServerDialog::createXModeBox()
{
    QGroupBox *box = new QGroupBox(tr("XMode options"));
    box->setEnabled(Config.GameMode == QStringLiteral("06_XMode"));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QComboBox *roleChooseComboBox = new QComboBox;
    roleChooseComboBox->addItem(tr("Normal"), QStringLiteral("Normal"));
    roleChooseComboBox->addItem(tr("Random"), QStringLiteral("Random"));
    roleChooseComboBox->addItem(tr("All roles"), QStringLiteral("AllRoles"));

    role_choose_xmode_ComboBox = roleChooseComboBox;

    QString scheme = Config.value(QStringLiteral("XMode/RoleChooseX"), QStringLiteral("Normal")).toString();
    if (scheme == QStringLiteral("Random"))
        roleChooseComboBox->setCurrentIndex(1);
    else if (scheme == QStringLiteral("AllRoles"))
        roleChooseComboBox->setCurrentIndex(2);

    box->setLayout(HLay(new QLabel(tr("Role choose")), role_choose_xmode_ComboBox));
    return box;
}

QGroupBox *ServerDialog::createHegemonyBox()
{
    QGroupBox *box = new QGroupBox(tr("Hegemony options"));
    box->setEnabled(Config.GameMode.startsWith(QStringLiteral("hegemony_")));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QComboBox *firstshow = new QComboBox;
    firstshow->addItem(tr("Not Used"), QStringLiteral("None"));
    firstshow->addItem(tr("Instant"), QStringLiteral("Instant"));
    firstshow->addItem(tr("Postponed"), QStringLiteral("Postponed"));
    hegemony_first_show = firstshow;
    if (Config.HegemonyFirstShowReward == QStringLiteral("Instant"))
        firstshow->setCurrentIndex(1);
    else if (Config.HegemonyFirstShowReward == QStringLiteral("Postponed"))
        firstshow->setCurrentIndex(2);

    QComboBox *companion = new QComboBox;
    companion->addItem(tr("Instant"), QStringLiteral("Instant"));
    companion->addItem(tr("Postponed"), QStringLiteral("Postponed"));
    hegemony_companion = companion;
    if (Config.HegemonyCompanionReward == QStringLiteral("Postponed"))
        companion->setCurrentIndex(1);

    QComboBox *halfhpdraw = new QComboBox;
    halfhpdraw->addItem(tr("Instant"), QStringLiteral("Instant"));
    halfhpdraw->addItem(tr("Postponed"), QStringLiteral("Postponed"));
    hegemony_half_hp_draw = halfhpdraw;
    if (Config.HegemonyHalfHpReward == QStringLiteral("Postponed"))
        halfhpdraw->setCurrentIndex(1);

    QComboBox *careeristkill = new QComboBox;
    careeristkill->addItem(tr("As Usual"), QStringLiteral("AsUsual"));
    careeristkill->addItem(tr("Always draw 3"), QStringLiteral("AlwaysDraw3"));
    hegemony_careerist_kill = careeristkill;
    if (Config.HegemonyCareeristKillReward == QStringLiteral("AlwaysDraw3"))
        careeristkill->setCurrentIndex(1);

    QFormLayout *l = new QFormLayout;
    l->addRow(tr("First Show Reward"), firstshow);
    l->addRow(tr("Companion Reward"), companion);
    l->addRow(tr("Half HP Draw Reward"), halfhpdraw);
    l->addRow(tr("Careerist Kill Reward"), careeristkill);

    box->setLayout(l);
    return box;
}

QGroupBox *ServerDialog::createGameModeBox()
{
    QGroupBox *mode_box = new QGroupBox(tr("Game mode"));
    mode_group = new QButtonGroup;

    QObjectList item_list;

    // normal modes
    QMap<QString, QString> modes = Sanguosha->getAvailableModes();
    QMapIterator<QString, QString> itor(modes);
    while (itor.hasNext()) {
        itor.next();

        QRadioButton *button = new QRadioButton(itor.value());
        button->setObjectName(itor.key());
        mode_group->addButton(button);

        if (itor.key() == Config.GameMode)
            button->setChecked(true);

        connect(button, &QAbstractButton::toggled, this, &ServerDialog::checkCurrentBtnIsHegemonyMode);

        if (itor.key() == QStringLiteral("02_1v1")) {
            QGroupBox *box = create1v1Box();
            connect(button, &QAbstractButton::toggled, box, &QWidget::setEnabled);

            item_list << button << box;
        } else if (itor.key() == QStringLiteral("06_3v3")) {
            QGroupBox *box = create3v3Box();
            connect(button, &QAbstractButton::toggled, box, &QWidget::setEnabled);

            item_list << button << box;
        } else if (itor.key() == QStringLiteral("06_XMode")) {
            QGroupBox *box = createXModeBox();
            connect(button, &QAbstractButton::toggled, box, &QWidget::setEnabled);

            item_list << button << box;
        } else if (itor.key() == QStringLiteral("hegemony_10")) {
            hegemonyBox = createHegemonyBox();

            item_list << button << hegemonyBox;
        } else {
            item_list << button;
        }
    }

    QVBoxLayout *left = new QVBoxLayout;
    QVBoxLayout *right = new QVBoxLayout;

    for (int i = 0; i < item_list.length(); i++) {
        QObject *item = item_list.at(i);
        QVBoxLayout *side = i < 14 ? left : right;

        if (item->isWidgetType()) {
            QWidget *widget = qobject_cast<QWidget *>(item);
            side->addWidget(widget);
        } else {
            QLayout *item_layout = qobject_cast<QLayout *>(item);
            side->addLayout(item_layout);
        }
    }

    left->addStretch();
    right->addStretch();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addLayout(left);
    layout->addLayout(right);

    mode_box->setLayout(layout);

    return mode_box;
}

QLayout *ServerDialog::createButtonLayout()
{
    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addStretch();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    QPushButton *cancel_button = new QPushButton(tr("Cancel"));

    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);

    connect(ok_button, &QAbstractButton::clicked, this, &ServerDialog::onOkButtonClicked);
    connect(cancel_button, &QAbstractButton::clicked, this, &QDialog::reject);

    return button_layout;
}

void ServerDialog::onDetectButtonClicked()
{
    QHostInfo vHostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    QList<QHostAddress> vAddressList = vHostInfo.addresses();
    foreach (QHostAddress address, vAddressList) {
        if (!address.isNull() && address != QHostAddress::LocalHost && address.protocol() == QAbstractSocket::IPv4Protocol) {
            address_edit->setText(address.toString());
            return;
        }
    }
}

void ServerDialog::onOkButtonClicked()
{
    accept();
}

Select3v3GeneralDialog::Select3v3GeneralDialog(QDialog *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Select generals in extend 3v3 mode"));
    QStringList ex_generalsList = Config.value(QStringLiteral("3v3/ExtensionGenerals")).toStringList();
    ex_generals = QSet<QString>(ex_generalsList.begin(), ex_generalsList.end());
    QVBoxLayout *layout = new QVBoxLayout;
    tab_widget = new QTabWidget;
    fillTabWidget();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, &QAbstractButton::clicked, this, &QDialog::accept);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);

    layout->addWidget(tab_widget);
    layout->addLayout(hlayout);
    setLayout(layout);
    setMinimumWidth(550);

    connect(this, &QDialog::accepted, this, &Select3v3GeneralDialog::save3v3Generals);
}

void Select3v3GeneralDialog::fillTabWidget()
{
    const QList<const Package *> &packages = Sanguosha->packanges();
    foreach (const Package *package, packages) {
        switch (package->type()) {
        case QSanguosha::GeneralPack:
        case QSanguosha::MixedPack: {
            QListWidget *list = new QListWidget;
            list->setViewMode(QListView::IconMode);
            list->setDragDropMode(QListView::NoDragDrop);
            fillListWidget(list, package);

            tab_widget->addTab(list, Sanguosha->translate(package->name()));
        }
        default:
            break;
        }
    }
}

void Select3v3GeneralDialog::fillListWidget(QListWidget *list, const Package *pack)
{
    QList<const General *> generals = pack->generals();
    foreach (const General *general, generals) {
        if (Sanguosha->isGeneralHidden(general->name()))
            continue;

        QListWidgetItem *item = new QListWidgetItem(list);
        item->setData(Qt::UserRole, general->name());
        item->setIcon(QIcon(G_ROOM_SKIN.getGeneralPixmap(general->name(), QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY, false)));

        bool checked = false;
        if (ex_generals.isEmpty()) {
            checked = (pack->name() == QStringLiteral("standard") || pack->name() == QStringLiteral("wind")) && general->name() != QStringLiteral("yuji");
        } else
            checked = ex_generals.contains(general->name());

        if (checked)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }

    QAction *action = new QAction(tr("Check/Uncheck all"), list);
    list->addAction(action);
    list->setContextMenuPolicy(Qt::ActionsContextMenu);
    list->setResizeMode(QListView::Adjust);

    connect(action, &QAction::triggered, this, &Select3v3GeneralDialog::toggleCheck);
}

void ServerDialog::setMiniCheckBox()
{
}

void ServerDialog::checkCurrentBtnIsHegemonyMode(bool v)
{
    QRadioButton *but = qobject_cast<QRadioButton *>(sender());
    if (but != nullptr && v)
        hegemonyBox->setEnabled(but->objectName().startsWith(QStringLiteral("hegemony_")));
}

void Select3v3GeneralDialog::toggleCheck()
{
    QWidget *widget = tab_widget->currentWidget();
    QListWidget *list = qobject_cast<QListWidget *>(widget);

    if (list == nullptr || list->item(0) == nullptr)
        return;

    bool checked = list->item(0)->checkState() != Qt::Checked;

    for (int i = 0; i < list->count(); i++)
        list->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void Select3v3GeneralDialog::save3v3Generals()
{
    ex_generals.clear();

    for (int i = 0; i < tab_widget->count(); i++) {
        QWidget *widget = tab_widget->widget(i);
        QListWidget *list = qobject_cast<QListWidget *>(widget);
        if (list != nullptr) {
            for (int j = 0; j < list->count(); j++) {
                QListWidgetItem *item = list->item(j);
                if (item->checkState() == Qt::Checked)
                    ex_generals << item->data(Qt::UserRole).toString();
            }
        }
    }

    QStringList list = ex_generals.values();
    QVariant data = QVariant::fromValue(list);
    Config.setValue(QStringLiteral("3v3/ExtensionGenerals"), data);
}

void ServerDialog::select3v3Generals()
{
    QDialog *dialog = new Select3v3GeneralDialog(this);
    dialog->exec();
}

bool ServerDialog::config()
{
    exec();

    if (result() != Accepted)
        return false;

    Config.ServerName = server_name_edit->text();
    Config.OperationTimeout = timeout_spinbox->value();
    Config.OperationNoLimit = nolimit_checkbox->isChecked();
    Config.RandomSeat = random_seat_checkbox->isChecked();
    Config.AssignLatestGeneral = assign_latest_general_checkbox->isChecked();
    Config.EnableCheat = enable_cheat_checkbox->isChecked();
    Config.FreeChoose = Config.EnableCheat && free_choose_checkbox->isChecked();
    Config.FreeAssignSelf = Config.EnableCheat && free_assign_self_checkbox->isChecked() && free_assign_checkbox->isEnabled();
    Config.ForbidSIMC = forbid_same_ip_checkbox->isChecked();
    Config.DisableChat = disable_chat_checkbox->isChecked();
    Config.Enable2ndGeneral = second_general_checkbox->isChecked();
    if (Config.MaxHpScheme == 0) {
        Config.PreventAwakenBelow3 = false;
    } else {
        Config.Scheme0Subtraction = 3;
        Config.PreventAwakenBelow3 = prevent_awaken_below3_checkbox->isChecked();
    }
    Config.Address = address_edit->text();
    Config.CountDownSeconds = game_start_spinbox->value();
    Config.NullificationCountDown = nullification_spinbox->value();
    Config.EnableMinimizeDialog = minimize_dialog_checkbox->isChecked();
    Config.EnableAI = ai_enable_checkbox->isChecked();
    Config.OriginAIDelay = ai_delay_spinbox->value();
    Config.AIDelay = Config.OriginAIDelay;
    Config.AIDelayAD = ai_delay_ad_spinbox->value();
    Config.AlterAIDelayAD = ai_delay_altered_checkbox->isChecked();
    Config.AIProhibitBlindAttack = ai_prohibit_blind_attack_checkbox->isChecked();
    Config.LimitRobot = limit_robot_checkbox->isChecked();
    Config.ServerPort = port_edit->text().toInt();
    Config.SurrenderAtDeath = surrender_at_death_checkbox->isChecked();
    Config.LuckCardLimitation = luck_card_spinbox->value();

    // game mode
    QString objname = mode_group->checkedButton()->objectName();

    Config.GameMode = objname;

    if (Config.GameMode.startsWith(QStringLiteral("hegemony_"))) {
        Config.HegemonyFirstShowReward = hegemony_first_show->itemData(hegemony_first_show->currentIndex()).toString();
        Config.HegemonyCompanionReward = hegemony_companion->itemData(hegemony_companion->currentIndex()).toString();
        Config.HegemonyHalfHpReward = hegemony_half_hp_draw->itemData(hegemony_half_hp_draw->currentIndex()).toString();
        Config.HegemonyCareeristKillReward = hegemony_careerist_kill->itemData(hegemony_careerist_kill->currentIndex()).toString();
    }

    Config.setValue(QStringLiteral("ServerName"), Config.ServerName);
    Config.setValue(QStringLiteral("GameMode"), Config.GameMode);
    Config.setValue(QStringLiteral("OperationTimeout"), Config.OperationTimeout);
    Config.setValue(QStringLiteral("OperationNoLimit"), Config.OperationNoLimit);
    Config.setValue(QStringLiteral("RandomSeat"), Config.RandomSeat);
    Config.setValue(QStringLiteral("AssignLatestGeneral"), Config.AssignLatestGeneral);
    Config.setValue(QStringLiteral("EnableCheat"), Config.EnableCheat);
    Config.setValue(QStringLiteral("FreeChoose"), Config.FreeChoose);
    Config.setValue(QStringLiteral("FreeAssign"), Config.EnableCheat && free_assign_checkbox->isChecked());
    Config.setValue(QStringLiteral("FreeAssignSelf"), Config.FreeAssignSelf);
    Config.setValue(QStringLiteral("PileSwappingLimitation"), pile_swapping_spinbox->value());
    Config.setValue(QStringLiteral("WithoutLordskill"), without_lordskill_checkbox->isChecked());
    Config.setValue(QStringLiteral("EnableSPConvert"), sp_convert_checkbox->isChecked());
    Config.setValue(QStringLiteral("MaxChoice"), maxchoice_spinbox->value());
    Config.setValue(QStringLiteral("GodLimit"), godlimit_spinbox->value());
    Config.setValue(QStringLiteral("LordMaxChoice"), lord_maxchoice_spinbox->value());
    Config.setValue(QStringLiteral("NonLordMaxChoice"), nonlord_maxchoice_spinbox->value());
    Config.setValue(QStringLiteral("ForbidSIMC"), Config.ForbidSIMC);
    Config.setValue(QStringLiteral("DisableChat"), Config.DisableChat);
    Config.setValue(QStringLiteral("Enable2ndGeneral"), Config.Enable2ndGeneral);
    Config.setValue(QStringLiteral("MaxHpScheme"), Config.MaxHpScheme);
    Config.setValue(QStringLiteral("Scheme0Subtraction"), Config.Scheme0Subtraction);
    Config.setValue(QStringLiteral("PreventAwakenBelow3"), Config.PreventAwakenBelow3);
    Config.setValue(QStringLiteral("CountDownSeconds"), game_start_spinbox->value());
    Config.setValue(QStringLiteral("NullificationCountDown"), nullification_spinbox->value());
    Config.setValue(QStringLiteral("EnableMinimizeDialog"), Config.EnableMinimizeDialog);
    Config.setValue(QStringLiteral("EnableAI"), Config.EnableAI);
    Config.setValue(QStringLiteral("OriginAIDelay"), Config.OriginAIDelay);
    Config.setValue(QStringLiteral("AlterAIDelayAD"), ai_delay_altered_checkbox->isChecked());
    Config.setValue(QStringLiteral("AIDelayAD"), Config.AIDelayAD);
    Config.setValue(QStringLiteral("AIProhibitBlindAttack"), Config.AIProhibitBlindAttack);
    Config.setValue(QStringLiteral("LimitRobot"), Config.LimitRobot);

    Config.setValue(QStringLiteral("SurrenderAtDeath"), Config.SurrenderAtDeath);
    Config.setValue(QStringLiteral("LuckCardLimitation"), Config.LuckCardLimitation);
    Config.setValue(QStringLiteral("ServerPort"), Config.ServerPort);
    Config.setValue(QStringLiteral("Address"), Config.Address);

    if (Config.GameMode.startsWith(QStringLiteral("hegemony_"))) {
        Config.setValue(QStringLiteral("HegemonyFirstShowReward"), Config.HegemonyFirstShowReward);
        Config.setValue(QStringLiteral("HegemonyCompanionReward"), Config.HegemonyCompanionReward);
        Config.setValue(QStringLiteral("HegemonyHalfHpReward"), Config.HegemonyHalfHpReward);
        Config.setValue(QStringLiteral("HegemonyCareeristKillReward"), Config.HegemonyCareeristKillReward);
    }

    QSet<QString> ban_packages;
    QList<QAbstractButton *> checkboxes = extension_group->buttons();
    foreach (QAbstractButton *checkbox, checkboxes) {
        if (!checkbox->isChecked()) {
            QString package_name = checkbox->objectName();
            Sanguosha->addBanPackage(package_name);
            ban_packages.insert(package_name);
        }
    }

    Config.BanPackages = ban_packages.values();
    Config.setValue(QStringLiteral("BanPackages"), Config.BanPackages);

    return true;
}

Server::Server(QObject *parent)
    : QObject(parent)
{
    server = new NativeServerSocket;
    server->setParent(this);

    // Synchonize ServerInfo and Config
    ServerInfo.DuringGame = true;
    ServerInfo.Name = Config.ServerName;
    ServerInfo.GameMode = Config.GameMode;
    if (Config.GameMode == QStringLiteral("02_1v1"))
        ServerInfo.GameRuleMode = Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString();
    else if (Config.GameMode == QStringLiteral("06_3v3"))
        ServerInfo.GameRuleMode = Config.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString();
    ServerInfo.OperationTimeout = Config.OperationNoLimit ? 0 : Config.OperationTimeout;
    ServerInfo.NullificationCountDown = Config.NullificationCountDown;

    const QList<const Package *> &packages = Sanguosha->packanges();
    foreach (const Package *package, packages) {
        QString package_name = package->name();
        // Why Sanguosha->getBanPackages() ????????
        if (Sanguosha->getBanPackages().contains(package_name))
            package_name = QStringLiteral("!") + package_name;

        ServerInfo.Extensions << package_name;
    }

    ServerInfo.RandomSeat = Config.RandomSeat;
    ServerInfo.EnableCheat = Config.EnableCheat;
    ServerInfo.FreeChoose = Config.FreeChoose;
    ServerInfo.Enable2ndGeneral = Config.Enable2ndGeneral;
    ServerInfo.EnableSame = Config.EnableSame;
    ServerInfo.DisableChat = Config.DisableChat;
    ServerInfo.MaxHpScheme = Config.MaxHpScheme;
    ServerInfo.Scheme0Subtraction = Config.Scheme0Subtraction;

    current = nullptr;
    createNewRoom();

    connect(server, &ServerSocket::new_connection, this, &Server::processNewConnection);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
}

void Server::broadcast(const QString &msg)
{
    QString to_sent = QString::fromUtf8(msg.toUtf8().toBase64());
    JsonArray arg;
    arg << QStringLiteral(".") << to_sent;

    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SPEAK);
    packet.setMessageBody(arg);
    foreach (Room *room, rooms)
        room->broadcastInvoke(&packet);
}

bool Server::listen()
{
    return server->listen();
}

void Server::daemonize()
{
    server->daemonize();
}

Room *Server::createNewRoom()
{
    Room *new_room = new Room(this, Config.GameMode);
    if (new_room->getLuaState() == nullptr) {
        delete new_room;
        return nullptr;
    }
    current = new_room;
    rooms.insert(current);

    connect(current, &Room::room_message, this, &Server::server_message);
    connect(current, &Room::game_over, this, &Server::gameOver);

    return current;
}

void Server::processNewConnection(ClientSocket *socket)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_CHECK_VERSION);
    packet.setMessageBody((Sanguosha->getVersion()));
    socket->send((packet.toString()));
    emit server_message(tr("%1 connected").arg(socket->peerName()));

    connect(socket, &ClientSocket::message_got, this, &Server::processRequest);
}

void Server::processRequest(const char *request)
{
    ClientSocket *socket = qobject_cast<ClientSocket *>(sender());
    socket->disconnect(this, SLOT(processRequest(const char *)));

    Packet signup;
    if (!signup.parse(request) || signup.getCommandType() != S_COMMAND_SIGNUP) {
        emit server_message(tr("Invalid signup string: %1").arg(QString::fromUtf8(request)));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody(QStringLiteral("INVALID_FORMAT"));
        socket->send(packet.toString());
        socket->disconnectFromHost();
        return;
    }

    const JsonArray &body = signup.getMessageBody().value<JsonArray>();
    QString urlPath = body[0].toString();
    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(body[1].toString().toLatin1()));
    QString avatar = body[2].toString();
    bool reconnection_enabled = false;

    QStringList ps = urlPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString messageBodyToSend;
    if (ps.length() == 0) {
        // default connected
    } else {
        if (ps.length() != 2) {
            messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            emit server_message(tr("invalid operation: more than 2 parts"));
        } else {
            // check valid ps.first
            if (ps.first() == QStringLiteral("reconnect")) {
                reconnection_enabled = true;
            } else if (ps.first() == QStringLiteral("observe")) {
                // warning, not implemented
                emit server_message(tr("unimplemented operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("OPERATION_NOT_IMPLEMENTED");
            } else {
                emit server_message(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            }
        }
        if (messageBodyToSend.isEmpty()) {
            // check valid ps.last
            if (!ps.last().startsWith(QStringLiteral("sgs"))) {
                emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
            } else {
                QString num = ps.last().mid(3);
                bool ok = false;
                num.toInt(&ok);
                if (ok) {
                    // valid connection name
                } else {
                    emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                    messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
                }
            }
        }

        if (!messageBodyToSend.isEmpty()) {
            QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
            packet.setMessageBody(messageBodyToSend);
            socket->send(packet.toString());
            socket->disconnectFromHost();
            return;
        }
    }
    if (Config.ForbidSIMC) {
        QString addr = socket->peerAddress();
        if (addresses.contains(addr)) {
            socket->disconnectFromHost();
            emit server_message(tr("Forbid the connection of address %1").arg(addr));
            return;
        } else {
            addresses.insert(addr);
            connect(socket, &ClientSocket::disconnected, this, &Server::cleanupSimc);
        }
    }

    Packet packet2(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QVariant s = ServerInfo.serialize();
    packet2.setMessageBody(s);
    socket->send((packet2.toString()));

    if (reconnection_enabled) {
        ServerPlayer *player = players.value(ps.last());
        if ((player != nullptr) && player->getState() == QStringLiteral("offline") && !player->getRoom()->isFinished()) {
            player->getRoom()->reconnect(player, socket);
            return;
        }

        // player not found
        emit server_message(tr("reconnect username not found: %1").arg(ps.last()));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody(QStringLiteral("USERNAME_INCORRECT"));
        socket->send(packet.toString());
        socket->disconnectFromHost();

        return;
    }

    if (current == nullptr || current->isFull() || current->isFinished())
        createNewRoom();

    ServerPlayer *player = current->addSocket(socket);
    current->signup(player, screen_name, avatar, false);
}

void Server::cleanupSimc()
{
    if (Config.ForbidSIMC) {
        const ClientSocket *socket = qobject_cast<const ClientSocket *>(sender());
        addresses.remove(socket->peerAddress());
    }
}

void Server::signupPlayer(ServerPlayer *player)
{
    name2objname.insert(player->screenName(), player->objectName());
    players.insert(player->objectName(), player);
}

void Server::gameOver()
{
    Room *room = qobject_cast<Room *>(sender());
    rooms.remove(room);

    foreach (ServerPlayer *player, room->findChildren<ServerPlayer *>()) {
        name2objname.remove(player->screenName(), player->objectName());
        players.remove(player->objectName());
    }
}
