#include "server.h"
#include "SkinBank.h"
#include "choosegeneraldialog.h"
#include "engine.h"
#include "nativesocket.h"
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
    connect(nolimit_checkbox, SIGNAL(toggled(bool)), timeout_spinbox, SLOT(setDisabled(bool)));

    // add 1v1 banlist edit button
    QPushButton *edit_button = new QPushButton(tr("Banlist ..."));
    edit_button->setFixedWidth(100);
    connect(edit_button, SIGNAL(clicked()), this, SLOT(edit1v1Banlist()));

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
    disable_lua_checkbox = new QCheckBox(tr("Disable Lua"));
    disable_lua_checkbox->setChecked(Config.DisableLua);
    disable_lua_checkbox->setToolTip(tr("<font color=#FFFF33>The setting takes effect after reboot</font>"));

    extension_group = new QButtonGroup;
    extension_group->setExclusive(false);

    QStringList extensions = Sanguosha->getExtensions();
    QSet<QString> ban_packages = Config.BanPackages.toSet();

    QGroupBox *box1 = new QGroupBox(tr("General package"));
    QGroupBox *box2 = new QGroupBox(tr("Card package"));

    QGridLayout *layout1 = new QGridLayout;
    QGridLayout *layout2 = new QGridLayout;
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    int i = 0, j = 0;
    int row = 0, column = 0;
    foreach (QString extension, extensions) {
        const Package *package = Sanguosha->findChild<const Package *>(extension);
        if (package == NULL)
            continue;

        bool forbid_package = Config.value("ForbidPackages").toStringList().contains(extension);
        QCheckBox *checkbox = new QCheckBox;
        checkbox->setObjectName(extension);
        checkbox->setText(Sanguosha->translate(extension));
        checkbox->setChecked(!ban_packages.contains(extension) && !forbid_package);
        checkbox->setEnabled(!forbid_package);

        extension_group->addButton(checkbox);

        switch (package->getType()) {
        case Package::GeneralPack: {
            if (extension == "standard" || extension == "test")
                continue;
            row = i / 5;
            column = i % 5;
            i++;

            layout1->addWidget(checkbox, row, column + 1);
            break;
        }
        case Package::CardPack: {
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
    layout->addWidget(disable_lua_checkbox);
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
    free_assign_checkbox->setChecked(Config.value("FreeAssign").toBool());
    free_assign_checkbox->setVisible(Config.EnableCheat);

    free_assign_self_checkbox = new QCheckBox(tr("Assign only your own role"));
    free_assign_self_checkbox->setChecked(Config.FreeAssignSelf);
    free_assign_self_checkbox->setEnabled(free_assign_checkbox->isChecked());
    free_assign_self_checkbox->setVisible(Config.EnableCheat);

    connect(enable_cheat_checkbox, SIGNAL(toggled(bool)), free_choose_checkbox, SLOT(setVisible(bool)));
    connect(enable_cheat_checkbox, SIGNAL(toggled(bool)), free_assign_checkbox, SLOT(setVisible(bool)));
    connect(enable_cheat_checkbox, SIGNAL(toggled(bool)), free_assign_self_checkbox, SLOT(setVisible(bool)));
    connect(free_assign_checkbox, SIGNAL(toggled(bool)), free_assign_self_checkbox, SLOT(setEnabled(bool)));

    pile_swapping_label = new QLabel(tr("Pile-swapping limitation"));
    pile_swapping_label->setToolTip(tr("<font color=#FFFF33>-1 means no limitations</font>"));
    pile_swapping_spinbox = new QSpinBox;
    pile_swapping_spinbox->setRange(-1, 15);
    pile_swapping_spinbox->setValue(Config.value("PileSwappingLimitation", 5).toInt());

    without_lordskill_checkbox = new QCheckBox(tr("Without Lordskill"));
    without_lordskill_checkbox->setChecked(Config.value("WithoutLordskill", false).toBool());

    sp_convert_checkbox = new QCheckBox(tr("Enable SP Convert"));
    sp_convert_checkbox->setChecked(Config.value("EnableSPConvert", true).toBool());

    maxchoice_spinbox = new QSpinBox;
    maxchoice_spinbox->setRange(3, 10);
    maxchoice_spinbox->setValue(Config.value("MaxChoice", 6).toInt());

    godlimit_label = new QLabel(tr("Upperlimit for gods"));
    godlimit_label->setToolTip(tr("<font color=#FFFF33>-1 means that all gods may appear in your general chosen dialog!</font>"));
    godlimit_spinbox = new QSpinBox;
    godlimit_spinbox->setRange(-1, 8);
    godlimit_spinbox->setValue(Config.value("GodLimit", 1).toInt());

    lord_maxchoice_label = new QLabel(tr("Upperlimit for lord"));
    lord_maxchoice_label->setToolTip(tr("<font color=#FFFF33>-1 means that all lords are available</font>"));
    lord_maxchoice_spinbox = new QSpinBox;
    lord_maxchoice_spinbox->setRange(-1, 10);
    lord_maxchoice_spinbox->setValue(Config.value("LordMaxChoice", 6).toInt());

    nonlord_maxchoice_spinbox = new QSpinBox;
    nonlord_maxchoice_spinbox->setRange(0, 10);
    nonlord_maxchoice_spinbox->setValue(Config.value("NonLordMaxChoice", 6).toInt());

    forbid_same_ip_checkbox = new QCheckBox(tr("Forbid same IP with multiple connection"));
    forbid_same_ip_checkbox->setChecked(Config.ForbidSIMC);

    disable_chat_checkbox = new QCheckBox(tr("Disable chat"));
    disable_chat_checkbox->setChecked(Config.DisableChat);

    second_general_checkbox = new QCheckBox(tr("Enable second general"));
    second_general_checkbox->setChecked(Config.Enable2ndGeneral);

    //same_checkbox = new QCheckBox(tr("Enable Same"));
    //same_checkbox->setChecked(Config.EnableSame);

    //max_hp_label = new QLabel(tr("Max HP scheme"));
    /* max_hp_scheme_ComboBox = new QComboBox;
    max_hp_scheme_ComboBox->addItem(tr("Sum - X"));
    max_hp_scheme_ComboBox->addItem(tr("Minimum"));
    max_hp_scheme_ComboBox->addItem(tr("Maximum"));
    max_hp_scheme_ComboBox->addItem(tr("Average"));
    max_hp_scheme_ComboBox->setCurrentIndex(Config.MaxHpScheme); */

    prevent_awaken_below3_checkbox = new QCheckBox(tr("Prevent maxhp being less than 3 for awaken skills"));
    prevent_awaken_below3_checkbox->setChecked(Config.PreventAwakenBelow3);
    //prevent_awaken_below3_checkbox->setEnabled(max_hp_scheme_ComboBox->currentIndex() != 0);
    prevent_awaken_below3_checkbox->setEnabled(false);

    //scheme0_subtraction_label = new QLabel(tr("Subtraction for scheme 0"));
    //scheme0_subtraction_label->setVisible(max_hp_scheme_ComboBox->currentIndex() == 0);
    /* scheme0_subtraction_spinbox = new QSpinBox;
    scheme0_subtraction_spinbox->setRange(-5, 12);
    scheme0_subtraction_spinbox->setValue(Config.Scheme0Subtraction);
    scheme0_subtraction_spinbox->setVisible(max_hp_scheme_ComboBox->currentIndex() == 0); */

    //connect(max_hp_scheme_ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setMaxHpSchemeBox()));

    //enable_surprising_generals_checkbox = new QCheckBox(tr("Enable surprising generals"));
    //enable_surprising_generals_checkbox->setChecked(Config.EnableSurprisingGenerals);

    address_edit = new QLineEdit;
    address_edit->setText(Config.Address);
    address_edit->setPlaceholderText(tr("Public IP or domain"));

    QPushButton *detect_button = new QPushButton(tr("Detect my WAN IP"));
    connect(detect_button, SIGNAL(clicked()), this, SLOT(onDetectButtonClicked()));

    port_edit = new QLineEdit;
    port_edit->setText(QString::number(Config.ServerPort));
    port_edit->setValidator(new QIntValidator(1, 9999, port_edit));

    layout->addWidget(forbid_same_ip_checkbox);
    layout->addWidget(disable_chat_checkbox);
    //layout->addWidget(random_seat_checkbox);
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
    //layout->addLayout(HLay(max_hp_label, max_hp_scheme_ComboBox));
    //layout->addLayout(HLay(scheme0_subtraction_label, scheme0_subtraction_spinbox));
    layout->addWidget(prevent_awaken_below3_checkbox);
    //layout->addLayout(HLay(basara_checkbox, hegemony_checkbox));
    //layout->addLayout(HLay(hegemony_maxchoice_label, hegemony_maxchoice_spinbox));
    //layout->addLayout(HLay(hegemony_maxshown_label, hegemony_maxshown_spinbox));
    //layout->addWidget(same_checkbox);
    //layout->addWidget(enable_surprising_generals_checkbox);
    layout->addLayout(HLay(new QLabel(tr("Address")), address_edit));
    layout->addWidget(detect_button);
    layout->addLayout(HLay(new QLabel(tr("Port")), port_edit));
    layout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    //max_hp_label->setVisible(Config.Enable2ndGeneral);
    //connect(second_general_checkbox, SIGNAL(toggled(bool)), max_hp_label, SLOT(setVisible(bool)));
    //max_hp_scheme_ComboBox->setVisible(Config.Enable2ndGeneral);
    //connect(second_general_checkbox, SIGNAL(toggled(bool)), max_hp_scheme_ComboBox, SLOT(setVisible(bool)));

    if (Config.Enable2ndGeneral) {
        //prevent_awaken_below3_checkbox->setVisible(max_hp_scheme_ComboBox->currentIndex() != 0);
        //scheme0_subtraction_label->setVisible(max_hp_scheme_ComboBox->currentIndex() == 0);
        //scheme0_subtraction_spinbox->setVisible(max_hp_scheme_ComboBox->currentIndex() == 0);
    } else {
        prevent_awaken_below3_checkbox->setVisible(false);
        //scheme0_subtraction_label->setVisible(false);
        //scheme0_subtraction_spinbox->setVisible(false);
    }
    //connect(second_general_checkbox, SIGNAL(toggled(bool)), this, SLOT(setMaxHpSchemeBox()));

    /*hegemony_maxchoice_label->setVisible(Config.EnableHegemony);
    connect(hegemony_checkbox, SIGNAL(toggled(bool)), hegemony_maxchoice_label, SLOT(setVisible(bool)));
    hegemony_maxchoice_spinbox->setVisible(Config.EnableHegemony);
    connect(hegemony_checkbox, SIGNAL(toggled(bool)), hegemony_maxchoice_spinbox, SLOT(setVisible(bool)));

    hegemony_maxshown_label->setVisible(Config.EnableHegemony);
    connect(hegemony_checkbox, SIGNAL(toggled(bool)), hegemony_maxshown_label, SLOT(setVisible(bool)));
    hegemony_maxshown_spinbox->setVisible(Config.EnableHegemony);
    connect(hegemony_checkbox, SIGNAL(toggled(bool)), hegemony_maxshown_spinbox, SLOT(setVisible(bool)));
    */
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
    //ai_enable_checkbox->setEnabled(false); // Force to enable AI for disabling it causes crashes!!

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
    connect(ai_delay_altered_checkbox, SIGNAL(toggled(bool)), ai_delay_ad_spinbox, SLOT(setEnabled(bool)));

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
    if (!button)
        return;
    if (button->objectName().contains("1v1") || button->objectName().contains("1v3")) {
        //basara_checkbox->setChecked(false);
        //basara_checkbox->setEnabled(false);
    } else {
        //basara_checkbox->setEnabled(true);
    }
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
        ban_list << "Roles"
                 << "HulaoPass";
    QVBoxLayout *layout = new QVBoxLayout;

    QTabWidget *tab = new QTabWidget;
    layout->addWidget(tab);
    connect(tab, SIGNAL(currentChanged(int)), this, SLOT(switchTo(int)));

    foreach (const QString &item, ban_list) {
        QWidget *apage = new QWidget;

        list = new QListWidget;
        list->setObjectName(item);

        QStringList banlist = Config.value(QString("Banlist/%1").arg(item)).toStringList();
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

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(this, SIGNAL(accepted()), this, SLOT(saveAll()));
    connect(remove, SIGNAL(clicked()), this, SLOT(doRemoveButton()));
    connect(add, SIGNAL(clicked()), this, SLOT(doAddButton()));

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
    connect(chooser, SIGNAL(general_chosen(QString)), this, SLOT(addGeneral(QString)));
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

    QStringList banlist = banset.toList();
    Config.setValue(QString("Banlist/%1").arg(ban_list.at(item)), QVariant::fromValue(banlist));
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
    box->setEnabled(Config.GameMode == "02_1v1");
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout;

    QComboBox *officialComboBox = new QComboBox;
    officialComboBox->addItem(tr("Classical"), "Classical");
    officialComboBox->addItem("2013", "2013");
    officialComboBox->addItem("OL", "OL");

    official_1v1_ComboBox = officialComboBox;

    QString rule = Config.value("1v1/Rule", "2013").toString();
    if (rule == "2013")
        officialComboBox->setCurrentIndex(1);
    else if (rule == "OL")
        officialComboBox->setCurrentIndex(2);

    //kof_using_extension_checkbox = new QCheckBox(tr("General extensions"));
    //kof_using_extension_checkbox->setChecked(Config.value("1v1/UsingExtension", false).toBool());

    //kof_card_extension_checkbox = new QCheckBox(tr("Card extensions"));
    //kof_card_extension_checkbox->setChecked(Config.value("1v1/UsingCardExtension", false).toBool());

    vlayout->addLayout(HLay(new QLabel(tr("Rule option")), official_1v1_ComboBox));

    /*QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(new QLabel(tr("Extension setting")));
    hlayout->addStretch();
    hlayout->addWidget(kof_using_extension_checkbox);
    hlayout->addWidget(kof_card_extension_checkbox);

    vlayout->addLayout(hlayout);*/
    box->setLayout(vlayout);

    return box;
}

QGroupBox *ServerDialog::create3v3Box()
{
    QGroupBox *box = new QGroupBox(tr("3v3 options"));
    box->setEnabled(Config.GameMode == "06_3v3");
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout;

    /*official_3v3_radiobutton = new QRadioButton(tr("Official mode"));

    QComboBox *officialComboBox = new QComboBox;
    officialComboBox->addItem(tr("Classical"), "Classical");
    officialComboBox->addItem("2012", "2012");
    officialComboBox->addItem("2013", "2013");

    official_3v3_ComboBox = officialComboBox;

    QString rule = Config.value("3v3/OfficialRule", "2013").toString();
    if (rule == "2012")
    officialComboBox->setCurrentIndex(1);
    else if (rule == "2013")
    officialComboBox->setCurrentIndex(2);
    */
    QRadioButton *extend = new QRadioButton(tr("Extension mode"));
    QPushButton *extend_edit_button = new QPushButton(tr("General selection ..."));
    extend_edit_button->setEnabled(false);
    connect(extend, SIGNAL(toggled(bool)), extend_edit_button, SLOT(setEnabled(bool)));
    connect(extend_edit_button, SIGNAL(clicked()), this, SLOT(select3v3Generals()));

    exclude_disaster_checkbox = new QCheckBox(tr("Exclude disasters"));
    exclude_disaster_checkbox->setChecked(Config.value("3v3/ExcludeDisasters", true).toBool());

    QComboBox *roleChooseComboBox = new QComboBox;
    roleChooseComboBox->addItem(tr("Normal"), "Normal");
    roleChooseComboBox->addItem(tr("Random"), "Random");
    roleChooseComboBox->addItem(tr("All roles"), "AllRoles");

    role_choose_ComboBox = roleChooseComboBox;

    QString scheme = Config.value("3v3/RoleChoose", "Normal").toString();
    if (scheme == "Random")
        roleChooseComboBox->setCurrentIndex(1);
    else if (scheme == "AllRoles")
        roleChooseComboBox->setCurrentIndex(2);

    //vlayout->addLayout(HLay(official_3v3_radiobutton, official_3v3_ComboBox));
    vlayout->addLayout(HLay(extend, extend_edit_button));
    vlayout->addWidget(exclude_disaster_checkbox);
    vlayout->addLayout(HLay(new QLabel(tr("Role choose")), role_choose_ComboBox));
    box->setLayout(vlayout);

    //bool using_extension = Config.value("3v3/UsingExtension", false).toBool();
    //if (using_extension)
    extend->setChecked(true);
    // else
    //     official_3v3_radiobutton->setChecked(true);

    return box;
}

QGroupBox *ServerDialog::createXModeBox()
{
    QGroupBox *box = new QGroupBox(tr("XMode options"));
    box->setEnabled(Config.GameMode == "06_XMode");
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QComboBox *roleChooseComboBox = new QComboBox;
    roleChooseComboBox->addItem(tr("Normal"), "Normal");
    roleChooseComboBox->addItem(tr("Random"), "Random");
    roleChooseComboBox->addItem(tr("All roles"), "AllRoles");

    role_choose_xmode_ComboBox = roleChooseComboBox;

    QString scheme = Config.value("XMode/RoleChooseX", "Normal").toString();
    if (scheme == "Random")
        roleChooseComboBox->setCurrentIndex(1);
    else if (scheme == "AllRoles")
        roleChooseComboBox->setCurrentIndex(2);

    box->setLayout(HLay(new QLabel(tr("Role choose")), role_choose_xmode_ComboBox));
    return box;
}

QGroupBox *ServerDialog::createHegemonyBox()
{
    QGroupBox *box = new QGroupBox(tr("Hegemony options"));
    box->setEnabled(Config.GameMode.startsWith("hegemony_"));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QComboBox *firstshow = new QComboBox;
    firstshow->addItem(tr("Not Used"), "None");
    firstshow->addItem(tr("Instant"), "Instant");
    firstshow->addItem(tr("Postponed"), "Postponed");
    hegemony_first_show = firstshow;
    if (Config.HegemonyFirstShowReward == "Instant")
        firstshow->setCurrentIndex(1);
    else if (Config.HegemonyFirstShowReward == "Postponed")
        firstshow->setCurrentIndex(2);

    QComboBox *companion = new QComboBox;
    companion->addItem(tr("Instant"), "Instant");
    companion->addItem(tr("Postponed"), "Postponed");
    hegemony_companion = companion;
    if (Config.HegemonyCompanionReward == "Postponed")
        companion->setCurrentIndex(1);

    QComboBox *halfhpdraw = new QComboBox;
    halfhpdraw->addItem(tr("Instant"), "Instant");
    halfhpdraw->addItem(tr("Postponed"), "Postponed");
    hegemony_half_hp_draw = halfhpdraw;
    if (Config.HegemonyHalfHpReward == "Postponed")
        halfhpdraw->setCurrentIndex(1);

    QComboBox *careeristkill = new QComboBox;
    careeristkill->addItem(tr("As Usual"), "AsUsual");
    careeristkill->addItem(tr("Always draw 3"), "AlwaysDraw3");
    hegemony_careerist_kill = careeristkill;
    if (Config.HegemonyCareeristKillReward == "AlwaysDraw3")
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

        connect(button, SIGNAL(toggled(bool)), this, SLOT(checkCurrentBtnIsHegemonyMode(bool)));

        if (itor.key() == "02_1v1") {
            QGroupBox *box = create1v1Box();
            connect(button, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));

            item_list << button << box;
        } else if (itor.key() == "06_3v3") {
            QGroupBox *box = create3v3Box();
            connect(button, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));

            item_list << button << box;
        } else if (itor.key() == "06_XMode") {
            QGroupBox *box = createXModeBox();
            connect(button, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));

            item_list << button << box;
        } else if (itor.key() == "hegemony_10") {
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

    connect(ok_button, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
    connect(cancel_button, SIGNAL(clicked()), this, SLOT(reject()));

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
    ex_generals = Config.value("3v3/ExtensionGenerals").toStringList().toSet();
    QVBoxLayout *layout = new QVBoxLayout;
    tab_widget = new QTabWidget;
    fillTabWidget();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);

    layout->addWidget(tab_widget);
    layout->addLayout(hlayout);
    setLayout(layout);
    setMinimumWidth(550);

    connect(this, SIGNAL(accepted()), this, SLOT(save3v3Generals()));
}

void Select3v3GeneralDialog::fillTabWidget()
{
    QList<const Package *> packages = Sanguosha->findChildren<const Package *>();
    foreach (const Package *package, packages) {
        switch (package->getType()) {
        case Package::GeneralPack:
        case Package::MixedPack: {
            QListWidget *list = new QListWidget;
            list->setViewMode(QListView::IconMode);
            list->setDragDropMode(QListView::NoDragDrop);
            fillListWidget(list, package);

            tab_widget->addTab(list, Sanguosha->translate(package->objectName()));
        }
        default:
            break;
        }
    }
}

void Select3v3GeneralDialog::fillListWidget(QListWidget *list, const Package *pack)
{
    QList<const General *> generals = pack->findChildren<const General *>();
    foreach (const General *general, generals) {
        if (Sanguosha->isGeneralHidden(general->objectName()))
            continue;

        QListWidgetItem *item = new QListWidgetItem(list);
        item->setData(Qt::UserRole, general->objectName());
        item->setIcon(QIcon(G_ROOM_SKIN.getGeneralPixmap(general->objectName(), QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY, false)));

        bool checked = false;
        if (ex_generals.isEmpty()) {
            checked = (pack->objectName() == "standard" || pack->objectName() == "wind") && general->objectName() != "yuji";
        } else
            checked = ex_generals.contains(general->objectName());

        if (checked)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }

    QAction *action = new QAction(tr("Check/Uncheck all"), list);
    list->addAction(action);
    list->setContextMenuPolicy(Qt::ActionsContextMenu);
    list->setResizeMode(QListView::Adjust);

    connect(action, SIGNAL(triggered()), this, SLOT(toggleCheck()));
}

void ServerDialog::setMiniCheckBox()
{
    //mini_scene_ComboBox->setEnabled(false);
}

void ServerDialog::checkCurrentBtnIsHegemonyMode(bool v)
{
    QRadioButton *but = qobject_cast<QRadioButton *>(sender());
    if (but != NULL && v)
        hegemonyBox->setEnabled(but->objectName().startsWith("hegemony_"));
}

void Select3v3GeneralDialog::toggleCheck()
{
    QWidget *widget = tab_widget->currentWidget();
    QListWidget *list = qobject_cast<QListWidget *>(widget);

    if (list == NULL || list->item(0) == NULL)
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
        if (list) {
            for (int j = 0; j < list->count(); j++) {
                QListWidgetItem *item = list->item(j);
                if (item->checkState() == Qt::Checked)
                    ex_generals << item->data(Qt::UserRole).toString();
            }
        }
    }

    QStringList list = ex_generals.toList();
    QVariant data = QVariant::fromValue(list);
    Config.setValue("3v3/ExtensionGenerals", data);
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
    //Config.EnableSame = same_checkbox->isChecked();
    //Config.EnableBasara = basara_checkbox->isChecked() && basara_checkbox->isEnabled();
    //Config.EnableHegemony = hegemony_checkbox->isChecked() && hegemony_checkbox->isEnabled();
    //Config.MaxHpScheme = max_hp_scheme_ComboBox->currentIndex();
    if (Config.MaxHpScheme == 0) {
        //Config.Scheme0Subtraction = scheme0_subtraction_spinbox->value();
        Config.PreventAwakenBelow3 = false;
    } else {
        Config.Scheme0Subtraction = 3;
        Config.PreventAwakenBelow3 = prevent_awaken_below3_checkbox->isChecked();
    }
    //Config.EnableSurprisingGenerals = enable_surprising_generals_checkbox->isChecked();
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
    Config.DisableLua = disable_lua_checkbox->isChecked();
    Config.SurrenderAtDeath = surrender_at_death_checkbox->isChecked();
    Config.LuckCardLimitation = luck_card_spinbox->value();

    // game mode
    QString objname = mode_group->checkedButton()->objectName();

    Config.GameMode = objname;

    if (Config.GameMode.startsWith("hegemony_")) {
        Config.HegemonyFirstShowReward = hegemony_first_show->itemData(hegemony_first_show->currentIndex()).toString();
        Config.HegemonyCompanionReward = hegemony_companion->itemData(hegemony_companion->currentIndex()).toString();
        Config.HegemonyHalfHpReward = hegemony_half_hp_draw->itemData(hegemony_half_hp_draw->currentIndex()).toString();
        Config.HegemonyCareeristKillReward = hegemony_careerist_kill->itemData(hegemony_careerist_kill->currentIndex()).toString();
    }

    Config.setValue("ServerName", Config.ServerName);
    Config.setValue("GameMode", Config.GameMode);
    Config.setValue("OperationTimeout", Config.OperationTimeout);
    Config.setValue("OperationNoLimit", Config.OperationNoLimit);
    Config.setValue("RandomSeat", Config.RandomSeat);
    Config.setValue("AssignLatestGeneral", Config.AssignLatestGeneral);
    Config.setValue("EnableCheat", Config.EnableCheat);
    Config.setValue("FreeChoose", Config.FreeChoose);
    Config.setValue("FreeAssign", Config.EnableCheat && free_assign_checkbox->isChecked());
    Config.setValue("FreeAssignSelf", Config.FreeAssignSelf);
    Config.setValue("PileSwappingLimitation", pile_swapping_spinbox->value());
    Config.setValue("WithoutLordskill", without_lordskill_checkbox->isChecked());
    Config.setValue("EnableSPConvert", sp_convert_checkbox->isChecked());
    Config.setValue("MaxChoice", maxchoice_spinbox->value());
    Config.setValue("GodLimit", godlimit_spinbox->value());
    Config.setValue("LordMaxChoice", lord_maxchoice_spinbox->value());
    Config.setValue("NonLordMaxChoice", nonlord_maxchoice_spinbox->value());
    Config.setValue("ForbidSIMC", Config.ForbidSIMC);
    Config.setValue("DisableChat", Config.DisableChat);
    Config.setValue("Enable2ndGeneral", Config.Enable2ndGeneral);
    //Config.setValue("EnableSame", Config.EnableSame);
    //Config.setValue("EnableSurprisingGenerals", Config.EnableSurprisingGenerals);
    Config.setValue("MaxHpScheme", Config.MaxHpScheme);
    Config.setValue("Scheme0Subtraction", Config.Scheme0Subtraction);
    Config.setValue("PreventAwakenBelow3", Config.PreventAwakenBelow3);
    Config.setValue("CountDownSeconds", game_start_spinbox->value());
    Config.setValue("NullificationCountDown", nullification_spinbox->value());
    Config.setValue("EnableMinimizeDialog", Config.EnableMinimizeDialog);
    Config.setValue("EnableAI", Config.EnableAI);
    Config.setValue("OriginAIDelay", Config.OriginAIDelay);
    Config.setValue("AlterAIDelayAD", ai_delay_altered_checkbox->isChecked());
    Config.setValue("AIDelayAD", Config.AIDelayAD);
    Config.setValue("AIProhibitBlindAttack", Config.AIProhibitBlindAttack);
    Config.setValue("LimitRobot", Config.LimitRobot);

    Config.setValue("SurrenderAtDeath", Config.SurrenderAtDeath);
    Config.setValue("LuckCardLimitation", Config.LuckCardLimitation);
    Config.setValue("ServerPort", Config.ServerPort);
    Config.setValue("Address", Config.Address);
    Config.setValue("DisableLua", disable_lua_checkbox->isChecked());
#if 0
    Config.beginGroup("3v3");
    //Config.setValue("UsingExtension", !official_3v3_radiobutton->isChecked());
    Config.setValue("RoleChoose", role_choose_ComboBox->itemData(role_choose_ComboBox->currentIndex()).toString());
    Config.setValue("ExcludeDisaster", exclude_disaster_checkbox->isChecked());
    //Config.setValue("OfficialRule", official_3v3_ComboBox->itemData(official_3v3_ComboBox->currentIndex()).toString());
    Config.endGroup();

    Config.beginGroup("1v1");
    Config.setValue("Rule", official_1v1_ComboBox->itemData(official_1v1_ComboBox->currentIndex()).toString());
    //Config.setValue("UsingExtension", kof_using_extension_checkbox->isChecked());
    //Config.setValue("UsingCardExtension", kof_card_extension_checkbox->isChecked());
    Config.endGroup();

    //Config.beginGroup("XMode");
    //Config.setValue("RoleChooseX", role_choose_xmode_ComboBox->itemData(role_choose_xmode_ComboBox->currentIndex()).toString());
    //Config.endGroup();
#endif
    if (Config.GameMode.startsWith("hegemony_")) {
        Config.setValue("HegemonyFirstShowReward", Config.HegemonyFirstShowReward);
        Config.setValue("HegemonyCompanionReward", Config.HegemonyCompanionReward);
        Config.setValue("HegemonyHalfHpReward", Config.HegemonyHalfHpReward);
        Config.setValue("HegemonyCareeristKillReward", Config.HegemonyCareeristKillReward);
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

    Config.BanPackages = ban_packages.toList();
    Config.setValue("BanPackages", Config.BanPackages);

    return true;
}

Server::Server(QObject *parent)
    : QObject(parent)
{
    server = new NativeServerSocket;
    server->setParent(this);

    //synchronize ServerInfo on the server side to avoid ambiguous usage of Config and ServerInfo
    ServerInfo.parse(Sanguosha->getSetupString());

    current = NULL;
    createNewRoom();

    connect(server, SIGNAL(new_connection(ClientSocket *)), this, SLOT(processNewConnection(ClientSocket *)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));
}

void Server::broadcast(const QString &msg)
{
    QString to_sent = msg.toUtf8().toBase64();
    JsonArray arg;
    arg << "." << to_sent;

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
    if (!new_room->getLuaState()) {
        delete new_room;
        return NULL;
    }
    current = new_room;
    rooms.insert(current);

    connect(current, SIGNAL(room_message(QString)), this, SIGNAL(server_message(QString)));
    connect(current, SIGNAL(game_over(QString)), this, SLOT(gameOver()));

    return current;
}

void Server::processNewConnection(ClientSocket *socket)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_CHECK_VERSION);
    packet.setMessageBody((Sanguosha->getVersion()));
    socket->send((packet.toString()));
    emit server_message(tr("%1 connected").arg(socket->peerName()));

    connect(socket, SIGNAL(message_got(const char *)), this, SLOT(processRequest(const char *)));
}

void Server::processRequest(const char *request)
{
    ClientSocket *socket = qobject_cast<ClientSocket *>(sender());
    socket->disconnect(this, SLOT(processRequest(const char *)));

    Packet signup;
    if (!signup.parse(request) || signup.getCommandType() != S_COMMAND_SIGNUP) {
        emit server_message(tr("Invalid signup string: %1").arg(request));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody("INVALID_FORMAT");
        socket->send(packet.toString());
        socket->disconnectFromHost();
        return;
    }

    const JsonArray &body = signup.getMessageBody().value<JsonArray>();
    QString urlPath = body[0].toString();
    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(body[1].toString().toLatin1()));
    QString avatar = body[2].toString();
    bool reconnection_enabled = false;

    QStringList ps = urlPath.split('/', QString::SkipEmptyParts);
    QString messageBodyToSend;
    if (ps.length() == 0) {
        // default connected
    } else {
        if (ps.length() != 2) {
            messageBodyToSend = "INVALID_OPERATION";
            emit server_message(tr("invalid operation: more than 2 parts"));
        } else {
            // check valid ps.first
            if (ps.first() == "reconnect") {
                reconnection_enabled = true;
            } else if (ps.first() == "observe") {
                // warning, not implemented
                emit server_message(tr("unimplemented operation: %1").arg(ps.first()));
                messageBodyToSend = "OPERATION_NOT_IMPLEMENTED";
            } else {
                emit server_message(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = "INVALID_OPERATION";
            }
        }
        if (messageBodyToSend.isEmpty()) {
            // check valid ps.last
            if (!ps.last().startsWith("sgs")) {
                emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                messageBodyToSend = "USERNAME_INCORRECT";
            } else {
                QString num = ps.last().mid(3);
                bool ok = false;
                num.toInt(&ok);
                if (ok) {
                    // valid connection name
                } else {
                    emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                    messageBodyToSend = "USERNAME_INCORRECT";
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
            connect(socket, SIGNAL(disconnected()), this, SLOT(cleanupSimc()));
        }
    }

    Packet packet2(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QString s = Sanguosha->getSetupString();
    packet2.setMessageBody(s);
    socket->send((packet2.toString()));

    if (reconnection_enabled) {
        ServerPlayer *player = players.value(ps.last());
        if (player && player->getState() == "offline" && !player->getRoom()->isFinished()) {
            player->getRoom()->reconnect(player, socket);
            return;
        }

        // player not found
        emit server_message(tr("reconnect username not found: %1").arg(ps.last()));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody("USERNAME_INCORRECT");
        socket->send(packet.toString());
        socket->disconnectFromHost();

        return;
    }

    if (current == NULL || current->isFull() || current->isFinished())
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
