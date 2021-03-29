#ifndef _SERVER_H
#define _SERVER_H

class Room;
class QGroupBox;
class QLabel;
class QRadioButton;

#include "clientstruct.h"
#include "detector.h"
#include "socket.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLayoutItem>
#include <QLineEdit>
#include <QListWidget>
#include <QMultiHash>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>

class Package;

class Select3v3GeneralDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Select3v3GeneralDialog(QDialog *parent);

private:
    QTabWidget *tab_widget;
    QSet<QString> ex_generals;

    void fillTabWidget();
    void fillListWidget(QListWidget *list, const Package *pack);

private slots:
    void save3v3Generals();
    void toggleCheck();
};

class BanlistDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BanlistDialog(QWidget *parent, bool view = false);

private:
    QList<QListWidget *> lists;
    QListWidget *list;
    int item;
    QStringList ban_list;

private slots:
    void addGeneral(const QString &name);
    void doAddButton();
    void doRemoveButton();
    void save();
    void saveAll();
    void switchTo(int item);
};

class ServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerDialog(QWidget *parent);
    bool config();

private:
    QWidget *createBasicTab();
    QWidget *createPackageTab();
    QWidget *createAdvancedTab();
    QWidget *createMiscTab();
    QLayout *createButtonLayout();

    QGroupBox *createGameModeBox();
    QGroupBox *create1v1Box();
    QGroupBox *create3v3Box();
    QGroupBox *createXModeBox();

    QGroupBox *createHegemonyBox();

    QLineEdit *server_name_edit;
    QSpinBox *timeout_spinbox;
    QCheckBox *nolimit_checkbox;
    QCheckBox *random_seat_checkbox;
    QCheckBox *assign_latest_general_checkbox;
    QCheckBox *enable_cheat_checkbox;
    QCheckBox *free_choose_checkbox;
    QCheckBox *free_assign_checkbox;
    QCheckBox *free_assign_self_checkbox;
    QLabel *pile_swapping_label;
    QSpinBox *pile_swapping_spinbox;
    QCheckBox *without_lordskill_checkbox;
    QCheckBox *sp_convert_checkbox;
    QSpinBox *maxchoice_spinbox;
    QSpinBox *godlimit_spinbox;
    QLabel *godlimit_label;
    QLabel *lord_maxchoice_label;
    QSpinBox *lord_maxchoice_spinbox;
    QSpinBox *nonlord_maxchoice_spinbox;
    QCheckBox *forbid_same_ip_checkbox;
    QCheckBox *disable_chat_checkbox;
    QCheckBox *second_general_checkbox;
    QCheckBox *scene_checkbox; //changjing
    QCheckBox *same_checkbox;
    QCheckBox *basara_checkbox;
    QCheckBox *hegemony_checkbox;
    QLabel *hegemony_maxchoice_label;
    QSpinBox *hegemony_maxchoice_spinbox;
    QLabel *hegemony_maxshown_label;
    QSpinBox *hegemony_maxshown_spinbox;
    QLabel *max_hp_label;
    QComboBox *max_hp_scheme_ComboBox;
    QLabel *scheme0_subtraction_label;
    QSpinBox *scheme0_subtraction_spinbox;
    QCheckBox *prevent_awaken_below3_checkbox;
    QLineEdit *address_edit;
    QLineEdit *port_edit;
    QSpinBox *game_start_spinbox;
    QSpinBox *nullification_spinbox;
    QCheckBox *minimize_dialog_checkbox;
    QCheckBox *ai_enable_checkbox;
    QSpinBox *ai_delay_spinbox;
    QCheckBox *ai_delay_altered_checkbox;
    QSpinBox *ai_delay_ad_spinbox;
    QCheckBox *ai_prohibit_blind_attack_checkbox;
    QCheckBox *limit_robot_checkbox;
    QCheckBox *surrender_at_death_checkbox;
    QLabel *luck_card_label;
    QSpinBox *luck_card_spinbox;
    QRadioButton *official_3v3_radiobutton;
    QComboBox *official_3v3_ComboBox;
    QComboBox *role_choose_ComboBox;
    QCheckBox *exclude_disaster_checkbox;
    QComboBox *official_1v1_ComboBox;
    QCheckBox *kof_using_extension_checkbox;
    QCheckBox *kof_card_extension_checkbox;
    QComboBox *role_choose_xmode_ComboBox;
    QCheckBox *disable_lua_checkbox;

    QCheckBox *enable_surprising_generals_checkbox;

    QButtonGroup *extension_group;
    QButtonGroup *mode_group;

    QGroupBox *hegemonyBox;
    QComboBox *hegemony_first_show;
    QComboBox *hegemony_companion;
    QComboBox *hegemony_half_hp_draw;
    QComboBox *hegemony_careerist_kill;

private slots:

    void onOkButtonClicked();
    void onDetectButtonClicked();
    void select3v3Generals();
    void edit1v1Banlist();
    void updateButtonEnablility(QAbstractButton *button);

    void setMiniCheckBox();

    void checkCurrentBtnIsHegemonyMode(bool v);
};

class ServerPlayer;

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent);

    void broadcast(const QString &msg);
    bool listen();
    void daemonize();
    Room *createNewRoom();
    void signupPlayer(ServerPlayer *player);

private:
    ServerSocket *server;
    Room *current;
    QSet<Room *> rooms;
    QHash<QString, ServerPlayer *> players;
    QSet<QString> addresses;
    QMultiHash<QString, QString> name2objname;

private slots:
    void processNewConnection(ClientSocket *socket);
    void processRequest(const char *request);
    void cleanupSimc();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
