#include "mainwindow.h"
#include "AboutUs.h"
#include "audio.h"
#include "cardoverview.h"
#include "client.h"
#include "configdialog.h"
#include "connectiondialog.h"
#include "generaloverview.h"
#include "lua.hpp"
#include "pixmapanimation.h"
#include "record-analysis.h"
#include "recorder.h"
#include "roomscene.h"
#include "server.h"
#include "sgswindow.h"
#include "startscene.h"
#include "ui_mainwindow.h"
#include "updatedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGroupBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTime>
#include <QToolButton>
#include <QVariant>
#include <QtMath>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

class FitView : public QGraphicsView
{
public:
    explicit FitView(QGraphicsScene *scene)
        : QGraphicsView(scene)
    {
        setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);
    }

    virtual void resizeEvent(QResizeEvent *event)
    {
        QGraphicsView::resizeEvent(event);
        MainWindow *main_window = qobject_cast<MainWindow *>(parentWidget());
        if (scene()->inherits("RoomScene")) {
            RoomScene *room_scene = qobject_cast<RoomScene *>(scene());
            QRectF newSceneRect(0, 0, 1080. * event->size().width() / event->size().height(), 1080);
            room_scene->setSceneRect(newSceneRect);
            room_scene->adjustItems();
            setSceneRect(room_scene->sceneRect());
            QTransform tf;
            tf.scale(1080. / event->size().height(), 1080. / event->size().height());
            setTransform(tf);
            fitInView(newSceneRect, Qt::KeepAspectRatio);
        } else if (scene()->inherits("StartScene")) {
            StartScene *start_scene = qobject_cast<StartScene *>(scene());
            QRectF newSceneRect(-event->size().width() / 2, -event->size().height() / 2, event->size().width(), event->size().height());
            start_scene->setSceneRect(newSceneRect);
            setSceneRect(start_scene->sceneRect());
            if (newSceneRect != start_scene->sceneRect())
                fitInView(start_scene->sceneRect(), Qt::KeepAspectRatio);
            if (main_window)
                main_window->setBackgroundBrush(true);
        }
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene = NULL;

    setWindowTitle(tr("TouhouSatsu") + "    " + Sanguosha->getVersionName() + "    " + Sanguosha->getVersionNumber());

    connection_dialog = new ConnectionDialog(this);
    connect(ui->actionStart_Game, SIGNAL(triggered()), connection_dialog, SLOT(exec()));
    connect(connection_dialog, SIGNAL(accepted()), this, SLOT(startConnection()));

    config_dialog = new ConfigDialog(this);
    connect(ui->actionConfigure, SIGNAL(triggered()), config_dialog, SLOT(show()));
    connect(config_dialog, SIGNAL(bg_changed()), this, SLOT(changeBackground()));
    connect(config_dialog, SIGNAL(tableBg_changed()), this, SLOT(changeTableBg()));

    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAcknowledgement_2, SIGNAL(triggered()), this, SLOT(on_actionAcknowledgement_triggered()));

    update_dialog = new UpdateDialog(this);

    StartScene *start_scene = new StartScene;
    //play title BGM
#ifdef AUDIO_SUPPORT
    if (Config.EnableBgMusic) {
        Audio::stopBGM();
        Audio::playBGM(Audio::getBgmFileNames(QString(), false));
        Audio::setBGMVolume(Config.BGMVolume);
    }
#endif
    QList<QAction *> actions;
    actions << ui->actionStart_Game << ui->actionStart_Server << ui->actionPC_Console_Start << ui->actionReplay << ui->actionGeneral_Overview << ui->actionCard_Overview
            << ui->actionConfigure << ui->actionAbout_Us;

    foreach (QAction *action, actions)
        start_scene->addButton(action);
    view = new FitView(scene);

    setCentralWidget(view);
    restoreFromConfig();

    BackLoader::preload();
    gotoScene(start_scene);

    addAction(ui->actionShow_Hide_Menu);
    addAction(ui->actionFullscreen);

    systray = NULL;

    if (Config.EnableAutoUpdate)
        update_dialog->checkForUpdate();
}

void MainWindow::restoreFromConfig()
{
    int width = Config.value("WindowWidth", 1366).toInt();
    int height = Config.value("WindowHeight", 706).toInt();
    int x = Config.value("WindowX", -8).toInt();
    int y = Config.value("WindowY", -8).toInt();
    bool maximized = Config.value("WindowMaximized", false).toBool();

    if (maximized)
        setWindowState(Qt::WindowMaximized);
    else {
        resize(QSize(width, height));
        move(x, y);
    }

    QFont font;
    if (Config.UIFont != font)
        QApplication::setFont(Config.UIFont, "QTextEdit");

    ui->actionEnable_Hotkey->setChecked(Config.EnableHotKey);
    ui->actionNever_nullify_my_trick->setChecked(Config.NeverNullifyMyTrick);
    ui->actionNever_nullify_my_trick->setEnabled(false);
}

void MainWindow::closeEvent(QCloseEvent *)
{
    Config.setValue("WindowWidth", width());
    Config.setValue("WindowHeight", height());
    Config.setValue("WindowX", x());
    Config.setValue("WindowY", y());
    Config.setValue("WindowMaximized", bool(windowState() & Qt::WindowMaximized));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::gotoScene(QGraphicsScene *scene)
{
    if (this->scene)
        this->scene->deleteLater();
    this->scene = scene;
    view->setScene(scene);
    /* @todo: Need a better way to replace the magic number '4' */
    //QResizeEvent e(QSize(view->size().width() - 4, view->size().height() - 4), view->size());
    QResizeEvent e(QSize(view->size().width(), view->size().height()), view->size());
    view->resizeEvent(&e);
    //play BGM
#ifdef AUDIO_SUPPORT
    if (Config.EnableBgMusic && !Audio::isBackgroundMusicPlaying()) {
        Audio::stopBGM();
        Audio::playBGM(Audio::getBgmFileNames(QString(), false));
        Audio::setBGMVolume(Config.BGMVolume);
    }
#endif
    changeBackground();
}

void MainWindow::on_actionExit_triggered()
{
    QMessageBox::StandardButton result;
    result = QMessageBox::question(this, tr("TouhouSatsu"), tr("Are you sure to exit?"), QMessageBox::Ok | QMessageBox::Cancel);
    if (result == QMessageBox::Ok) {
        delete systray;
        systray = NULL;
        close();
    }
}

void MainWindow::on_actionStart_Server_triggered()
{
    ServerDialog *dialog = new ServerDialog(this);
    if (!dialog->config())
        return;

    Server *server = new Server(this);
    if (!server->listen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Can not start server!"));
        return;
    }

    server->daemonize();

    ui->actionStart_Game->disconnect();
    connect(ui->actionStart_Game, SIGNAL(triggered()), this, SLOT(startGameInAnotherInstance()));

    StartScene *start_scene = qobject_cast<StartScene *>(scene);
    if (start_scene) {
        start_scene->switchToServer(server);
        if (Config.value("EnableMinimizeDialog", false).toBool())
            on_actionMinimize_to_system_tray_triggered();
    }
}

void MainWindow::checkVersion(const QString &server_version, const QString &server_mod)
{
    Client *client = qobject_cast<Client *>(sender());

    QString client_mod = Sanguosha->getMODName();
    if (client_mod != server_mod) {
        client->disconnectFromHost();
        QMessageBox::warning(this, tr("Warning"), tr("Client MOD name is not same as the server!"));
        return;
    }
    QString client_version = Sanguosha->getVersionNumber();

    if (server_version == client_version) {
        client->signup();
        connect(client, SIGNAL(server_connected()), SLOT(enterRoom()));
        return;
    }

    client->disconnectFromHost();

    QString text = tr("Server version is %1, client version is %2 <br/>").arg(server_version).arg(client_version);
    if (server_version > client_version)
        text.append(tr("Your client version is older than the server's, please update it <br/>"));
    else
        text.append(tr("The server version is older than your client version, please ask the server to update<br/>"));

    if (!Config.EnableAutoUpdate)
        text.append(tr("Enable auto update from the config dialog, and restart the game to check update."));
    else if (Config.AutoUpdateNeedsRestart) {
        if (Config.AutoUpdateDataRececived)
            text.append(tr("An error occurred when parsing update info. Please restart the game and retry auto updating."));
        else
            text.append(tr("Please restart the game and try auto updating."));
    } else if (!Config.AutoUpdateDataRececived)
        text.append(tr("Please wait a minute for downloading update info."));
    else
        text.append(tr("It seems like your version is the latest version. Either the server is using a test version, or auto updater is not up-to-date."));

    QMessageBox::warning(this, tr("Warning"), text);
}

void MainWindow::startConnection()
{
    Client *client = new Client(this);

    connect(client, SIGNAL(version_checked(QString, QString)), SLOT(checkVersion(QString, QString)));
    connect(client, SIGNAL(error_message(QString)), SLOT(networkError(QString)));
}

void MainWindow::on_actionReplay_triggered()
{
    QString location = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString last_dir = Config.value("LastReplayDir").toString();
    if (!last_dir.isEmpty())
        location = last_dir;

    QString filename = QFileDialog::getOpenFileName(this, tr("Select a reply file"), location, tr("Pure text replay file (*.txt);; Image replay file (*.png)"));

    if (filename.isEmpty())
        return;

    QFileInfo file_info(filename);
    last_dir = file_info.absoluteDir().path();
    Config.setValue("LastReplayDir", last_dir);

    Client *client = new Client(this, filename);
    connect(client, SIGNAL(server_connected()), SLOT(enterRoom()));
    client->signup();
}

void MainWindow::networkError(const QString &error_msg)
{
    if (isVisible())
        QMessageBox::warning(this, tr("Network error"), error_msg);
}

void BackLoader::preload()
{
    QStringList emotions = G_ROOM_SKIN.getAnimationFileNames();

    foreach (QString emotion, emotions) {
        int n = PixmapAnimation::GetFrameCount(emotion);
        for (int i = 0; i < n; i++) {
            QString filename = QString("image/system/emotion/%1/%2.png").arg(emotion).arg(QString::number(i));
            G_ROOM_SKIN.getPixmapFromFileName(filename);
        }
    }
}

void MainWindow::enterRoom()
{
    if (QUrl(Config.HostAddress).path().length() == 0) {
        // add current ip to history only if the modifiers does not exist.
        // add the last connected address to the first one. DO NOT SORT
        if (Config.HistoryIPs.contains(Config.HostAddress))
            Config.HistoryIPs.removeAll(Config.HostAddress);
        Config.HistoryIPs.prepend(Config.HostAddress);
        Config.setValue("HistoryUrls", Config.HistoryIPs);
    }

    ui->actionStart_Game->setEnabled(false);
    ui->actionStart_Server->setEnabled(false);

    RoomScene *room_scene = new RoomScene(this);
    ui->actionView_Discarded->setEnabled(true);
    ui->actionView_distance->setEnabled(true);
    ui->actionServerInformation->setEnabled(true);
    ui->actionSurrender->setEnabled(true);
    ui->actionNever_nullify_my_trick->setEnabled(true);
    ui->actionSaveRecord->setEnabled(true);

    connect(ClientInstance, SIGNAL(surrender_enabled(bool)), ui->actionSurrender, SLOT(setEnabled(bool)));

    connect(ui->actionView_Discarded, SIGNAL(triggered()), room_scene, SLOT(toggleDiscards()));
    connect(ui->actionView_distance, SIGNAL(triggered()), room_scene, SLOT(viewDistance()));
    connect(ui->actionServerInformation, SIGNAL(triggered()), room_scene, SLOT(showServerInformation()));
    connect(ui->actionSurrender, SIGNAL(triggered()), room_scene, SLOT(surrender()));
    connect(ui->actionSaveRecord, SIGNAL(triggered()), room_scene, SLOT(saveReplayRecord()));

    if (ServerInfo.EnableCheat) {
        ui->menuCheat->setEnabled(true);

        connect(ui->actionDeath_note, SIGNAL(triggered()), room_scene, SLOT(makeKilling()));
        connect(ui->actionDamage_maker, SIGNAL(triggered()), room_scene, SLOT(makeDamage()));
        connect(ui->actionRevive_wand, SIGNAL(triggered()), room_scene, SLOT(makeReviving()));
        connect(ui->actionExecute_script_at_server_side, SIGNAL(triggered()), room_scene, SLOT(doScript()));
    } else {
        ui->menuCheat->setEnabled(false);
        ui->actionDeath_note->disconnect();
        ui->actionDamage_maker->disconnect();
        ui->actionRevive_wand->disconnect();
        ui->actionExecute_script_at_server_side->disconnect();
    }

    connect(room_scene, SIGNAL(restart()), this, SLOT(startConnection()));
    connect(room_scene, SIGNAL(return_to_start()), this, SLOT(gotoStartScene()));

    gotoScene(room_scene);
}

void MainWindow::gotoStartScene()
{
    //play BGM
#ifdef AUDIO_SUPPORT
    if (Config.EnableBgMusic && !Audio::isBackgroundMusicPlaying()) {
        Audio::stopBGM();
        Audio::playBGM(Audio::getBgmFileNames(QString(), false));
        Audio::setBGMVolume(Config.BGMVolume);
    }
#endif
    ServerInfo.DuringGame = false;
    QList<Server *> servers = findChildren<Server *>();
    if (!servers.isEmpty())
        servers.first()->deleteLater();

    StartScene *start_scene = new StartScene;

    QList<QAction *> actions;
    actions << ui->actionStart_Game << ui->actionStart_Server << ui->actionPC_Console_Start << ui->actionReplay << ui->actionGeneral_Overview << ui->actionCard_Overview
            << ui->actionConfigure << ui->actionAbout_Us;

    foreach (QAction *action, actions)
        start_scene->addButton(action);

    setCentralWidget(view);

    ui->menuCheat->setEnabled(false);
    ui->actionDeath_note->disconnect();
    ui->actionDamage_maker->disconnect();
    ui->actionRevive_wand->disconnect();
    ui->actionExecute_script_at_server_side->disconnect();
    gotoScene(start_scene);

    addAction(ui->actionShow_Hide_Menu);
    addAction(ui->actionFullscreen);

    delete systray;
    systray = NULL;
    if (ClientInstance) {
        if (Self) {
            delete Self;
            Self = NULL;
        }
        delete ClientInstance;
        ClientInstance = NULL;
    }
}

void MainWindow::startGameInAnotherInstance()
{
    QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
}

void MainWindow::on_actionGeneral_Overview_triggered()
{
    GeneralOverview *overview = GeneralOverview::getInstance(this);
    overview->fillGenerals(Sanguosha->findChildren<const General *>());
    overview->show();
}

void MainWindow::on_actionCard_Overview_triggered()
{
    CardOverview *overview = CardOverview::getInstance(this);
    overview->loadFromAll();
    overview->show();
}

void MainWindow::on_actionEnable_Hotkey_toggled(bool checked)
{
    if (Config.EnableHotKey != checked) {
        Config.EnableHotKey = checked;
        Config.setValue("EnableHotKey", checked);
    }
}

void MainWindow::on_actionNever_nullify_my_trick_toggled(bool checked)
{
    if (Config.NeverNullifyMyTrick != checked) {
        Config.NeverNullifyMyTrick = checked;
        Config.setValue("NeverNullifyMyTrick", checked);
    }
}

void MainWindow::on_actionAbout_triggered()
{
    // Cao Cao's pixmap
    QString content = "<center><img src='image/system/shencc.png'> <br /> </center>";

    // Cao Cao' poem
    QString poem = tr("Disciples dressed in blue, my heart worries for you. You are the cause, of this song without pause");
    content.append(QString("<p align='right'><i>%1</i></p>").arg(poem));

    // Cao Cao's signature
    QString signature = tr("\"A Short Song\" by Cao Cao");
    content.append(QString("<p align='right'><i>%1</i></p>").arg(signature));

    //QString email = "moligaloo@gmail.com";
    //content.append(tr("This is the open source clone of the popular <b>Sanguosha</b> game,"
    //    "totally written in C++ Qt GUI framework <br />"
    //    "My Email: <a href='mailto:%1' style = \"color:#0072c1; \">%1</a> <br/>"
    //    "My QQ: 365840793 <br/>"
    //    "My Weibo: http://weibo.com/moligaloo <br/>").arg(email));
    content.append(tr("This is the open source clone of the popular <b>Sanguosha</b> game,"
                      "totally written in C++ Qt GUI framework <br />"));
    //"My QQ: 384318315 <br/>"
    QString config;

#ifdef QT_NO_DEBUG
    config = "release";
#else
    config = "debug";
#endif

    content.append(tr("Current version: %1 %2 (%3)<br/>").arg(Sanguosha->getVersion()).arg(config).arg(Sanguosha->getVersionName()));

    const char *date = __DATE__;
    const char *time = __TIME__;
    content.append(tr("Compilation time: %1 %2 <br/>").arg(date).arg(time));

    QString project_url = "https://github.com/lwtmusou/touhoukill";
    content.append(tr("Source code: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(project_url));

    QString forum_url = "http://qsanguosha.org";
    content.append(tr("Forum: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(forum_url));

    Window *window = new Window(tr("About QSanguosha"), QSize(420, 465));
    scene->addItem(window);
    window->setZValue(32766);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->shift(scene->inherits("RoomScene") ? scene->width() : 0, scene->inherits("RoomScene") ? scene->height() : 0);

    window->appear();
}

void MainWindow::on_actionAbout_Us_triggered()
{
    AboutUsDialog *dialog = new AboutUsDialog(this);
    dialog->show();
}

void MainWindow::setBackgroundBrush(bool centerAsOrigin)
{
    if (scene) {
        QPixmap pixmap(Config.BackgroundImage);
        QBrush brush(pixmap);
        qreal sx = (qreal)width() / qreal(pixmap.width());
        qreal sy = (qreal)height() / qreal(pixmap.height());

        QTransform transform;
        if (centerAsOrigin)
            transform.translate(-(qreal)width() / 2, -(qreal)height() / 2);
        transform.scale(sx, sy);
        brush.setTransform(transform);
        scene->setBackgroundBrush(brush);
    }
}

void MainWindow::changeBackground()
{
    bool centerAsOrigin = scene != NULL && !scene->inherits("RoomScene");
    setBackgroundBrush(centerAsOrigin);

    if (scene->inherits("StartScene")) {
        StartScene *start_scene = qobject_cast<StartScene *>(scene);
        start_scene->setServerLogBackground();
    }
}

void MainWindow::changeTableBg()
{
    if (!scene->inherits("RoomScene"))
        return;

    RoomSceneInstance->changeTableBg();
}

void MainWindow::on_actionFullscreen_triggered()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void MainWindow::on_actionShow_Hide_Menu_triggered()
{
    QMenuBar *menu_bar = menuBar();
    menu_bar->setVisible(!menu_bar->isVisible());
}

void MainWindow::on_actionMinimize_to_system_tray_triggered()
{
    if (systray == NULL) {
        QIcon icon("image/system/magatamas/5.png");
        systray = new QSystemTrayIcon(icon, this);

        QAction *appear = new QAction(tr("Show main window"), this);
        connect(appear, SIGNAL(triggered()), this, SLOT(show()));

        QMenu *menu = new QMenu;
        menu->addAction(appear);
        menu->addMenu(ui->menuGame);
        menu->addMenu(ui->menuView);
        menu->addMenu(ui->menuOptions);
        menu->addMenu(ui->menuHelp);

        systray->setContextMenu(menu);
    }

    systray->show();
    systray->showMessage(windowTitle(), tr("Game is minimized"));

    hide();
}

void MainWindow::on_actionRole_assign_table_triggered()
{
    QString content;

    QStringList headers;
    headers << tr("Count") << tr("Lord") << tr("Loyalist") << tr("Rebel") << tr("Renegade");
    foreach (QString header, headers)
        content += QString("<th>%1</th>").arg(header);

    content = QString("<tr>%1</tr>").arg(content);

    QStringList rows;
    rows << "2 1 0 1 0"
         << "3 1 0 1 1"
         << "4 1 0 2 1"
         << "5 1 1 2 1"
         << "6 1 1 3 1"
         << "6d 1 1 2 2"
         << "7 1 2 3 1"
         << "8 1 2 4 1"
         << "8d 1 2 3 2"
         << "8z 1 3 4 0"
         << "9 1 3 4 1"
         << "10 1 3 4 2"
         << "10z 1 4 5 0"
         << "10o 1 3 5 1";

    foreach (QString row, rows) {
        QStringList cells = row.split(" ");
        QString header = cells.takeFirst();
        if (header.endsWith("d")) {
            header.chop(1);
            header += tr(" (double renegade)");
        }
        if (header.endsWith("z")) {
            header.chop(1);
            header += tr(" (no renegade)");
        }
        if (header.endsWith("o")) {
            header.chop(1);
            header += tr(" (single renegade)");
        }

        QString row_content;
        row_content = QString("<td>%1</td>").arg(header);
        foreach (QString cell, cells)
            row_content += QString("<td>%1</td>").arg(cell);

        content += QString("<tr>%1</tr>").arg(row_content);
    }

    content = QString("<table border='1'>%1</table").arg(content);

    Window *window = new Window(tr("Role assign table"), QSize(240, 450));
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->shift(scene && scene->inherits("RoomScene") ? scene->width() : 0, scene && scene->inherits("RoomScene") ? scene->height() : 0);
    window->setZValue(32766);

    window->appear();
}

BroadcastBox::BroadcastBox(Server *server, QWidget *parent)
    : QDialog(parent)
    , server(server)
{
    setWindowTitle(tr("Broadcast"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Please input the message to broadcast")));

    text_edit = new QTextEdit;
    layout->addWidget(text_edit);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    QPushButton *ok_button = new QPushButton(tr("OK"));
    hlayout->addWidget(ok_button);

    layout->addLayout(hlayout);

    setLayout(layout);

    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
}

void BroadcastBox::accept()
{
    QDialog::accept();
    server->broadcast(text_edit->toPlainText());
}

void MainWindow::on_actionBroadcast_triggered()
{
    Server *server = findChild<Server *>();
    if (server == NULL) {
        QMessageBox::warning(this, tr("Warning"), tr("Server is not started yet!"));
        return;
    }

    BroadcastBox *dialog = new BroadcastBox(server, this);
    dialog->exec();
}

void MainWindow::on_actionAcknowledgement_triggered()
{
    Window *window = new Window(QString(), QSize(1000, 677), "image/system/acknowledgement.png");
    scene->addItem(window);

    Button *button = window->addCloseButton(tr("OK"));
    button->moveBy(-85, -35);
    window->setZValue(32766);
    window->shift(scene && scene->inherits("RoomScene") ? scene->width() : 0, scene && scene->inherits("RoomScene") ? scene->height() : 0);

    window->appear();
}

void MainWindow::on_actionPC_Console_Start_triggered()
{
    ServerDialog *dialog = new ServerDialog(this);
    if (!dialog->config())
        return;

    Server *server = new Server(this);
    if (!server->listen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Can not start server!"));
        return;
    }

    server->createNewRoom();

    Config.HostAddress = "qths://127.0.0.1";
    startConnection();
}

void MainWindow::on_actionReplay_file_convert_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Please select a replay file"), Config.value("LastReplayDir").toString(),
                                                    tr("Pure text replay file (*.txt);; Image replay file (*.png)"));

    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QFileInfo info(filename);
        QString tosave = info.absoluteDir().absoluteFilePath(info.baseName());

        if (filename.endsWith(".txt")) {
            tosave.append(".png");

            // txt to png
            Recorder::TXT2PNG(file.readAll()).save(tosave);

        } else if (filename.endsWith(".png")) {
            tosave.append(".txt");

            // png to txt
            QByteArray data = Recorder::PNG2TXT(filename);

            QFile tosave_file(tosave);
            if (tosave_file.open(QIODevice::WriteOnly))
                tosave_file.write(data);
        }
    }
}

void MainWindow::on_actionRecord_analysis_triggered()
{
    QString location = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString filename = QFileDialog::getOpenFileName(this, tr("Load replay record"), location, tr("Pure text replay file (*.txt);; Image replay file (*.png)"));

    if (filename.isEmpty())
        return;

    QDialog *rec_dialog = new QDialog(this);
    rec_dialog->setWindowTitle(tr("Record Analysis"));
    rec_dialog->resize(800, 500);
    QTableWidget *table = new QTableWidget;

    RecAnalysis *record = new RecAnalysis(filename);
    QMap<QString, PlayerRecordStruct *> record_map = record->getRecordMap();
    table->setColumnCount(11);
    table->setRowCount(record_map.keys().length());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    static QStringList labels;
    if (labels.isEmpty()) {
        labels << tr("ScreenName") << tr("General") << tr("Role") << tr("Living") << tr("WinOrLose") << tr("TurnCount") << tr("Recover") << tr("Damage") << tr("Damaged")
               << tr("Kill");
    }
    table->setHorizontalHeaderLabels(labels);
    table->setSelectionBehavior(QTableWidget::SelectRows);

    int i = 0;
    foreach (PlayerRecordStruct *rec, record_map.values()) {
        QTableWidgetItem *item = new QTableWidgetItem;
        QString screen_name = Sanguosha->translate(rec->m_screenName);
        if (rec->m_statue == "robot")
            screen_name += "(" + Sanguosha->translate("robot") + ")";

        item->setText(screen_name);
        table->setItem(i, 0, item);

        item = new QTableWidgetItem;
        QString generals = Sanguosha->translate(rec->m_generalName);
        if (!rec->m_general2Name.isEmpty())
            generals += "/" + Sanguosha->translate(rec->m_general2Name);
        item->setText(generals);
        table->setItem(i, 1, item);

        item = new QTableWidgetItem;
        item->setText(Sanguosha->translate(rec->m_role));
        table->setItem(i, 2, item);

        item = new QTableWidgetItem;
        item->setText(rec->m_isAlive ? tr("Alive") : tr("Dead"));
        table->setItem(i, 3, item);

        item = new QTableWidgetItem;
        bool is_win = record->getRecordWinners().contains(rec->m_role) || record->getRecordWinners().contains(record_map.key(rec));
        item->setText(is_win ? tr("Win") : tr("Lose"));
        table->setItem(i, 4, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_turnCount));
        table->setItem(i, 5, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_recover));
        table->setItem(i, 6, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damage));
        table->setItem(i, 7, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damaged));
        table->setItem(i, 8, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_kill));
        table->setItem(i, 9, item);
        i++;
    }

    table->resizeColumnsToContents();

    QLabel *label = new QLabel;
    label->setText(tr("Packages:") + record->getRecordPackages().join(","));

    QLabel *label_game_mode = new QLabel;
    label_game_mode->setText(tr("GameMode:") + Sanguosha->getModeName(record->getRecordGameMode()));

    QLabel *label_options = new QLabel;
    label_options->setText(tr("ServerOptions:") + record->getRecordServerOptions().join(","));

    QTextEdit *chat_info = new QTextEdit;
    chat_info->setReadOnly(chat_info);
    chat_info->setText(record->getRecordChat());

    QLabel *table_chat_title = new QLabel;
    table_chat_title->setText(tr("Chat Information:"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(label_game_mode);
    layout->addWidget(label_options);
    layout->addWidget(table);
    layout->addSpacing(15);
    layout->addWidget(table_chat_title);
    layout->addWidget(chat_info);
    rec_dialog->setLayout(layout);

    rec_dialog->exec();
}

void MainWindow::on_actionView_ban_list_triggered()
{
    BanlistDialog *dialog = new BanlistDialog(this, true);
    dialog->exec();
}

void MainWindow::on_actionAbout_Lua_triggered()
{
    QString content = tr("Lua is a powerful, fast, lightweight, embeddable scripting language.");
    content.append("<p align='center'> <img src='image/logo/lua.png' /> </p> <br/>");

    QString address = "http://www.lua.org";
    content.append(tr("Official site: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(address));

    content.append(tr("Current versionn %1 <br/>").arg(LUA_RELEASE));
    content.append(LUA_COPYRIGHT);

    Window *window = new Window(tr("About Lua"), QSize(500, 585));
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->setZValue(32766);
    window->shift(scene && scene->inherits("RoomScene") ? scene->width() : 0, scene && scene->inherits("RoomScene") ? scene->height() : 0);

    window->appear();
}

void MainWindow::on_actionAbout_GPLv3_triggered()
{
    QString content = tr(
        "The GNU General Public License is the most widely used free software license, which guarantees end users the freedoms to use, study, share, and modify the software.");
    content.append("<p align='center'> <img src='image/logo/gplv3.png' /> </p> <br/>");

    QString address = "http://gplv3.fsf.org";
    content.append(tr("Official site: <a href='%1' style = \"color:#0072c1; \">%1</a> <br/>").arg(address));

    Window *window = new Window(tr("About GPLv3"), QSize(500, 225));
    scene->addItem(window);

    window->addContent(content);
    window->addCloseButton(tr("OK"));
    window->setZValue(32766);
    window->shift(scene && scene->inherits("RoomScene") ? scene->width() : 0, scene && scene->inherits("RoomScene") ? scene->height() : 0);

    window->appear();
}

// ATTENTION!!!! this slot is for "Download/update contents" menu item
void MainWindow::on_actionDownload_Hero_Skin_and_BGM_triggered()
{
    if (!Config.EnableAutoUpdate) {
        QMessageBox::warning(this, tr("TouhouSatsu"), tr("Please enable auto update, restart the game and retry."));
        return;
    } else if (!Config.AutoUpdateDataRececived) {
        if (Config.AutoUpdateNeedsRestart) {
            QMessageBox::warning(this, tr("TouhouSatsu"), tr("Please restart the game and retry."));
            return;
        } else {
            QMessageBox::information(this, tr("TouhouSatsu"), tr("Please wait a minute for downloading update info."));
            return;
        }
    } else {
        if (Config.AutoUpdateNeedsRestart) {
            QMessageBox::warning(this, tr("TouhouSatsu"), tr("An error occurred when parsing update info. Please restart the game and retry."));
            return;
        } else {
            update_dialog->exec();
        }
    }
}
