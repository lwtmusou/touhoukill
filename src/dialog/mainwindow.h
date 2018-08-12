#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "configdialog.h"
#include "connectiondialog.h"
#include "engine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QJsonObject>
#include <QMainWindow>
#include <QNetworkReply>
#include <QSettings>
#include <QSpinBox>

namespace Ui {
class MainWindow;
}

class FitView;
class QGraphicsScene;
class QSystemTrayIcon;
class Server;
class QTextEdit;
class QToolButton;
class QGroupBox;
class RoomItem;
class QProgressBar;
class QLabel;
class QWinTaskbarButton;

class BroadcastBox : public QDialog
{
    Q_OBJECT

public:
    BroadcastBox(Server *server, QWidget *parent = 0);

protected:
    virtual void accept();

private:
    Server *server;
    QTextEdit *text_edit;
};

class BackLoader
{
public:
    static void preload();
};

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateDialog(QWidget *parent = 0);
#if QT_VERSION >= 0x050600
    void setInfo(const QString &v, const QVersionNumber &vn, const QString &updateScript, const QString &updatePack, const QJsonObject &updateHash);
#else
    void setInfo(const QString &v, const QString &vn, const QString &updateScript, const QString &updatePack, const QJsonObject &updateHash);
#endif

private:
    QProgressBar *bar;
    QLabel *lbl;
    QNetworkAccessManager *downloadManager;
    QNetworkReply *scriptReply;
    QNetworkReply *packReply;
    QWinTaskbarButton *taskbarButton;

    QString m_updateScript;
    QString m_updatePack;
    QJsonObject m_updateHash;

    bool m_finishedScript;
    bool m_finishedPack;

    bool m_busy;

    void startUpdate();
    bool packHashVerify(const QByteArray &arr);

private slots:
    void startDownload();
    void downloadProgress(quint64 downloaded, quint64 total);
    void finishedScript();
    void errScript();
    void finishedPack();
    void errPack();

public slots:
    void accept() override;
    void reject() override;

protected:
    void showEvent(QShowEvent *event) override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setBackgroundBrush(bool center_as_origin);

protected:
    virtual void closeEvent(QCloseEvent *);

private:
    FitView *view;
    QGraphicsScene *scene;
    Ui::MainWindow *ui;
    ConnectionDialog *connection_dialog;
    ConfigDialog *config_dialog;
    QSystemTrayIcon *systray;
    QNetworkAccessManager *autoUpdateManager;

    void restoreFromConfig();
    void checkForUpdate();

#if QT_VERSION >= 0x050600
    void parseUpdateInfo(const QString &v, const QVersionNumber &vn, const QJsonObject &ob);
#else
    void parseUpdateInfo(const QString &v, const QString &vn, const QJsonObject &ob);
#endif
public slots:
    void startConnection();

private slots:
    void on_actionAbout_GPLv3_triggered();
    void on_actionAbout_Lua_triggered();
    void on_actionAbout_fmod_triggered();
    void on_actionReplay_file_convert_triggered();
    void on_actionPC_Console_Start_triggered();
    void on_actionRecord_analysis_triggered();
    void on_actionAcknowledgement_triggered();
    void on_actionBroadcast_triggered();
    void on_actionRole_assign_table_triggered();
    void on_actionMinimize_to_system_tray_triggered();
    void on_actionShow_Hide_Menu_triggered();
    void on_actionFullscreen_triggered();
    void on_actionReplay_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Us_triggered();
    void on_actionEnable_Hotkey_toggled(bool);
    void on_actionNever_nullify_my_trick_toggled(bool);
    void on_actionCard_Overview_triggered();
    void on_actionGeneral_Overview_triggered();
    void on_actionStart_Server_triggered();
    void on_actionExit_triggered();

    void checkVersion(const QString &server_version, const QString &server_mod);
    void networkError(const QString &error_msg);
    void enterRoom();
    void gotoScene(QGraphicsScene *scene);
    void gotoStartScene();
    void startGameInAnotherInstance();
    void changeBackground();
    void changeTableBg();
    void on_actionView_ban_list_triggered();

    void updateError(QNetworkReply::NetworkError e);
    void updateInfoReceived();
};

#endif
