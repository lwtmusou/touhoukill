#include "connectiondialog.h"
#include "SkinBank.h"
#include "detector.h"
#include "engine.h"
#include "settings.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QWidget>

AvatarModel::AvatarModel(const QList<const General *> &list)
    : list(list)
{
}

int AvatarModel::rowCount(const QModelIndex &) const
{
    return list.size();
}

QVariant AvatarModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= list.length())
        return QVariant();

    const General *general = list.at(row);

    switch (role) {
    case Qt::UserRole:
        return general->objectName();
    case Qt::DisplayRole:
        return Sanguosha->translate(general->objectName());
    case Qt::DecorationRole: {
        QIcon icon(G_ROOM_SKIN.getGeneralPixmap(general->objectName(), QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
        return icon;
    }
    }

    return QVariant();
}

void ConnectionDialog::hideAvatarList()
{
    if (!avatarList->isVisible())
        return;
    avatarList->hide();
    setFixedSize(shrinkSize);
}

void ConnectionDialog::showAvatarList()
{
    if (avatarList->isVisible())
        return;

    setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    if (avatarList->model() == NULL) {
        QList<const General *> generals = Sanguosha->findChildren<const General *>();
        QMutableListIterator<const General *> itor = generals;
        while (itor.hasNext()) {
            if (itor.next()->isTotallyHidden())
                itor.remove();
        }

        AvatarModel *model = new AvatarModel(generals);
        model->setParent(this);
        avatarList->setModel(model);
    }
    avatarList->show();
    if (expandSize.isEmpty())
        expandSize = size();

    setFixedSize(expandSize);
}

void ConnectionDialog::accept()
{
    QString username = nameLineEdit->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    QUrl url(hostComboBox->currentText());
    if (url.isValid() && (url.scheme() == "qths") && !url.host().isEmpty()) {
        // modifiers
        if (!url.path().isEmpty()) {
            // now we are planning to support the following 2 modifiers:
            // 1. reconnect
            // 2. observe
            //
            // The "reconnect" modifier should do the same as the reconnect function before
            // The "observe" modifier is .....

            QString p = url.path();
            QStringList ps = p.split('/', QString::SkipEmptyParts);
            if (ps.length() != 2) {
                QMessageBox::warning(this, tr("Warning"), tr("This pattern is not supported, please recheck your input."));
                return;
            } else {
                // check valid ps.first
                if (ps.first() == "reconnect") {
                    // correct reconnection indicator
                } else if (ps.first() == "observe") {
                    // warning, not implemented
                    QMessageBox::warning(this, tr("Warning"), tr("This operation is not implemented yet. Sorry for my laziness."));
                    return;
                } else {
                    QMessageBox::warning(this, tr("Warning"), tr("This operation is not supported, please recheck your input."));
                    return;
                }

                // check valid ps.last
                if (!ps.last().startsWith("sgs")) {
                    QMessageBox::warning(this, tr("Warning"), tr("The connection name is incorrect, please recheck your input."));
                    return;
                }

                QString num = ps.last().mid(3);
                bool ok = false;
                num.toInt(&ok);
                if (ok) {
                    // valid connection name
                } else {
                    QMessageBox::warning(this, tr("Warning"), tr("The connection name is incorrect, please recheck your input."));
                    return;
                }
            }
        }
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("Please input valid host address!<br />Starts with 0.9.7 the address must begin with \"qths://\"."));
        return;
    }

    if (username == tr("Sanguosha-fans")) {
        QMessageBox::StandardButton ask = QMessageBox::information(this, tr("Notice"),
                                                                   tr("You are meant to join the game using the default nickname.<br />"
                                                                      "We suggest that you change the nickname to identify the unique you for convenience.<br /><br />"
                                                                      "Do you want to change the nickname now?"),
                                                                   QMessageBox::Yes, QMessageBox::No);
        if (ask == QMessageBox::Yes)
            return;
    }

    Config.UserName = username;
    Config.HostAddress = hostComboBox->currentText();

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostUrl", Config.HostAddress);

    QDialog::accept();
}

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connection setup"));

    QGroupBox *gb = new QGroupBox(tr("Connection setup"));

    nameLineEdit = new QLineEdit;
    hostComboBox = new QComboBox;

    QFormLayout *connlayout = new QFormLayout;
    connlayout->addRow(tr("Name:"), nameLineEdit);
    connlayout->addRow(tr("Host:"), hostComboBox);

    QLabel *avalbl = new QLabel(tr("Avatar:"));
    avatarPixmap = new QLabel;
    avatarPixmap->setScaledContents(true);
    avatarPixmap->setMinimumSize(QSize(134, 134));
    avatarPixmap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout *avalayout = new QVBoxLayout;
    avalayout->addWidget(avalbl);
    avalayout->addWidget(avatarPixmap);
    avalayout->addStretch();

    QPushButton *fillreconnectinfo = new QPushButton(tr("Fill Reconnection Information"));
    connect(fillreconnectinfo, &QPushButton::clicked, this, &ConnectionDialog::on_fillreconnect_clicked);
    QPushButton *landetect = new QPushButton(tr("Detect LAN ..."));
    connect(landetect, &QPushButton::clicked, this, &ConnectionDialog::on_detectLANButton_clicked);
    QPushButton *clearhist = new QPushButton(tr("Clear history"));
    connect(clearhist, &QPushButton::clicked, this, &ConnectionDialog::on_clearHistoryButton_clicked);
    QPushButton *changeava = new QPushButton(tr("Change avatar"));
    connect(changeava, &QPushButton::clicked, this, &ConnectionDialog::on_changeAvatarButton_clicked);

    QVBoxLayout *btnlayout = new QVBoxLayout;
    btnlayout->addWidget(fillreconnectinfo);
    btnlayout->addWidget(landetect);
    btnlayout->addWidget(clearhist);
    btnlayout->addStretch();
    btnlayout->addWidget(changeava);

    QHBoxLayout *btmlayout = new QHBoxLayout;
    btmlayout->addLayout(avalayout);
    btmlayout->addLayout(btnlayout);
    QVBoxLayout *gblayout = new QVBoxLayout;
    gblayout->addLayout(connlayout);
    gblayout->addLayout(btmlayout);

    gb->setLayout(gblayout);

    QPushButton *connectbtn = new QPushButton(tr("Connect"));
    connect(connectbtn, &QPushButton::clicked, this, &ConnectionDialog::accept);
    connectbtn->setDefault(true);

    QPushButton *cancelbtn = new QPushButton(tr("Cancel"));
    connect(cancelbtn, &QPushButton::clicked, this, &ConnectionDialog::reject);

    QHBoxLayout *btnglayout = new QHBoxLayout;
    btnglayout->addStretch();
    btnglayout->addWidget(connectbtn);
    btnglayout->addWidget(cancelbtn);

    QVBoxLayout *llayout = new QVBoxLayout;
    llayout->addWidget(gb);
    llayout->addLayout(btnglayout);

    avatarList = new QListView;
    avatarList->setIconSize(QSize(80, 80));
    avatarList->setViewMode(QListView::IconMode);
    avatarList->setUniformItemSizes(true);
    avatarList->setMinimumWidth(500);
    avatarList->hide();
    connect(avatarList, &QListView::doubleClicked, this, &ConnectionDialog::on_avatarList_doubleClicked);

    QHBoxLayout *totlayout = new QHBoxLayout;
    totlayout->addLayout(llayout);
    totlayout->addWidget(avatarList);

    setLayout(totlayout);

    connectbtn->setFocus();
}

ConnectionDialog::~ConnectionDialog()
{
}

void ConnectionDialog::showEvent(QShowEvent *e)
{
    nameLineEdit->setText(Config.UserName);

    hostComboBox->setEditable(true);
    hostComboBox->clear();
    hostComboBox->addItems(Config.HistoryIPs);

    // if the last connected address has modifiers, don't use
    if (QUrl(Config.HostAddress).path().isEmpty())
        hostComboBox->setEditText(Config.HostAddress);
    else
        hostComboBox->setEditText(Config.HistoryIPs.first());

    avatarPixmap->setPixmap(G_ROOM_SKIN.getGeneralPixmap(Config.UserAvatar, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE, false));

    QDialog::showEvent(e);
    if (shrinkSize.isEmpty())
        shrinkSize = size();

    setFixedSize(shrinkSize);
}

void ConnectionDialog::on_changeAvatarButton_clicked()
{
    if (avatarList->isVisible()) {
        QModelIndex index = avatarList->currentIndex();
        if (index.isValid())
            on_avatarList_doubleClicked(index);
        else
            hideAvatarList();
    } else
        showAvatarList();
}

void ConnectionDialog::on_fillreconnect_clicked()
{
    QUrl u(hostComboBox->currentText());
    if (u.isValid() && (u.scheme() == "qths") && !u.host().isEmpty()) {
        u.setPath(QString(QStringLiteral("/reconnect/")).append(Config.value("LastSelfObjectName", QString(QStringLiteral("sgs1"))).toString()));
        hostComboBox->setEditText(u.toString());
    } else
        QMessageBox::warning(this, tr("Warning"), tr("Please fill the server information before we fill reconnection information for you."));
}

void ConnectionDialog::on_avatarList_doubleClicked(const QModelIndex &index)
{
    QString general_name = avatarList->model()->data(index, Qt::UserRole).toString();
    QPixmap avatar(G_ROOM_SKIN.getGeneralPixmap(general_name, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
    avatarPixmap->setPixmap(avatar);
    Config.UserAvatar = general_name;
    Config.setValue("UserAvatar", general_name);
    hideAvatarList();
}

void ConnectionDialog::on_clearHistoryButton_clicked()
{
    hostComboBox->clear();
    hostComboBox->setEditText("qths://");

    Config.HistoryIPs.clear();
    Config.remove("HistoryUrls");
}

void ConnectionDialog::on_detectLANButton_clicked()
{
    UdpDetectorDialog *detector_dialog = new UdpDetectorDialog(this);
    connect(detector_dialog, &UdpDetectorDialog::address_chosen, hostComboBox, &QComboBox::setEditText);

    detector_dialog->exec();
}

// -----------------------------------

UdpDetectorDialog::UdpDetectorDialog(QDialog *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Detect available server's addresses at LAN"));
    detect_button = new QPushButton(tr("Refresh"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(detect_button);

    list = new QListWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(list);
    layout->addLayout(hlayout);

    setLayout(layout);

    detector = NULL;
    connect(detect_button, SIGNAL(clicked()), this, SLOT(startDetection()));
    connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(chooseAddress(QListWidgetItem *)));

    detect_button->click();
}

void UdpDetectorDialog::startDetection()
{
    list->clear();
    detect_button->setEnabled(false);

    detector = new UdpDetector;
    connect(detector, SIGNAL(detected(QString, QString)), this, SLOT(addServerAddress(QString, QString)));
    QTimer::singleShot(2000, this, SLOT(stopDetection()));

    detector->detect();
}

void UdpDetectorDialog::stopDetection()
{
    detect_button->setEnabled(true);
    detector->stop();
    delete detector;
    detector = NULL;
}

void UdpDetectorDialog::addServerAddress(const QString &server_name, const QString &address_)
{
    QString address = address_;
    if (address.startsWith("::ffff:"))
        address.remove(0, 7);
    QString label = QString("%1 [%2]").arg(server_name).arg(address);
    QListWidgetItem *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, address);

    list->addItem(item);
}

void UdpDetectorDialog::chooseAddress(QListWidgetItem *item)
{
    accept();

    QString address = item->data(Qt::UserRole).toString();
    address.prepend("qths://");
    emit address_chosen(address);
}
