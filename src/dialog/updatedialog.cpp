#include "updatedialog.h"
#include "engine.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>

#if QT_VERSION >= 0x050600
#include <QVersionNumber>
#endif
#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

UpdateDialog::UpdateDialog(QWidget *parent)
    : QDialog(parent)
    , bar(new QProgressBar)
    , lbl(new QLabel)
    , downloadManager(new QNetworkAccessManager(this))
    , scriptReply(NULL)
    , packReply(NULL)
    , taskbarButton(NULL)
    , m_finishedScript(false)
    , m_finishedPack(false)
    , m_busy(false)
{
    setWindowTitle(tr("New Version Available"));

    bar->setMaximum(10000);

    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget(lbl);
    layout->addWidget(bar);

    QPushButton *yesBtn = new QPushButton(tr("Yes"));
    connect(yesBtn, &QPushButton::clicked, [yesBtn]() -> void { yesBtn->setDisabled(true); });

    QPushButton *noBtn = new QPushButton(tr("No"));
    connect(noBtn, &QPushButton::clicked, [this]() -> void { QDialog::reject(); });
    connect(yesBtn, &QPushButton::clicked, [noBtn]() -> void { noBtn->setDisabled(true); });

    connect(yesBtn, &QPushButton::clicked, this, &UpdateDialog::startDownload);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(yesBtn);
    hlayout->addWidget(noBtn);

    layout->addLayout(hlayout);

    setLayout(layout);
}

#if QT_VERSION >= 0x050600
void UpdateDialog::setInfo(const QString &v, const QVersionNumber &vn, const QString &updateScript, const QString &updatePack, const QJsonObject &updateHash)
#else
void UpdateDialog::setInfo(const QString &v, const QString &vn, const QString &updateScript, const QString &updatePack, const QJsonObject &updateHash)
#endif
{
    lbl->setText(tr("New Version %1(%3) available.\n"
                    "We support auto-updating from %2 to %1 on this platform.\n"
                    "Click 'Yes' to update now.")
                     .arg(v)
                     .arg(Sanguosha->getVersionNumber())
#if QT_VERSION >= 0x050600
                     .arg(vn.toString()));
#else
                     .arg(vn));
#endif

    m_updateScript = updateScript;
    m_updatePack = updatePack;
    m_updateHash = updateHash;
}

void UpdateDialog::startUpdate()
{
#ifdef Q_OS_WIN
    taskbarButton->progress()->hide();
#endif
// we should run update script and then exit this main program.
#if defined(Q_OS_WIN)
    QStringList arg;
    arg << "UpdateScript.vbs" << QString::number(QCoreApplication::applicationPid());
    QProcess::startDetached("wscript", arg, QCoreApplication::applicationDirPath());
#elif defined(Q_OS_ANDROID)
// call JNI to install the package
#else
    QStringList arg;
    arg << "-c" << ("\"./UpdateScript.sh " + QString::number(QCoreApplication::applicationPid()) + "\"");
    QProcess::startDetached("sh", arg, QCoreApplication::applicationDirPath());
#endif

    QCoreApplication::exit(0);
    QDialog::accept();
}

bool UpdateDialog::packHashVerify(const QByteArray &arr)
{
    static const QMap<QString, QCryptographicHash::Algorithm> algorithms {std::make_pair<QString, QCryptographicHash::Algorithm>("MD5", QCryptographicHash::Md5),
                                                                          std::make_pair<QString, QCryptographicHash::Algorithm>("SHA1", QCryptographicHash::Sha1)};

    foreach (const QString &str, algorithms.keys()) {
        if (m_updateHash.contains(str)) {
            QString hash = m_updateHash.value(str).toString();
            QByteArray calculatedHash = QCryptographicHash::hash(arr, algorithms.value(str));
            if (hash.toUpper() != QString::fromLatin1(calculatedHash.toHex()).toUpper())
                return false;
        }
    }

    return true;
}

void UpdateDialog::startDownload()
{
    if (m_updatePack.isEmpty() || m_updateScript.isEmpty()) {
        QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nURL is empty."));
        QDialog::reject();
        return;
    }

    m_busy = true;

    QNetworkRequest reqPack;
#if QT_VERSION >= 0x050600
    reqPack.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    reqPack.setUrl(QUrl(m_updatePack));
    packReply = downloadManager->get(reqPack);
    connect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
    connect(packReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
    connect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);

#ifndef Q_OS_ANDROID
    QNetworkRequest reqScript;
#if QT_VERSION >= 0x050600
    reqScript.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    reqScript.setUrl(QUrl(m_updateScript));
    scriptReply = downloadManager->get(reqScript);
    connect(scriptReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
    connect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
#else
    m_finishedScript = true;
#endif

#ifdef Q_OS_WIN
    taskbarButton->progress()->reset();
    taskbarButton->progress()->show();
#endif
}

void UpdateDialog::downloadProgress(quint64 downloaded, quint64 total)
{
    bar->setValue(10000 * downloaded / total);
#ifdef Q_OS_WIN
    taskbarButton->progress()->setValue(10000 * downloaded / total);
#endif
}

void UpdateDialog::finishedScript()
{
#if defined(Q_OS_WIN)
    QString suffix = ".vbs";
#else
    QString suffix = ".sh";
#endif
    QByteArray arr = scriptReply->readAll();
    QFile file;
    file.setFileName(QString("UpdateScript") + suffix);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(arr);
    file.close();

#ifdef Q_OS_UNIX
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);
#endif

    m_finishedScript = true;
    if (m_finishedPack && m_finishedScript)
        startUpdate();
}

void UpdateDialog::errScript()
{
#ifdef Q_OS_WIN
    taskbarButton->progress()->hide();
#endif
    if (scriptReply != NULL) {
        disconnect(scriptReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
        disconnect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
    }
    if (packReply != NULL) {
        disconnect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
        disconnect(packReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
        disconnect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);
    }

    QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nCannot download the update script."));
    QDialog::reject();
}

void UpdateDialog::finishedPack()
{
#if defined(Q_OS_WIN)
    QString suffix = ".7z";
#elif defined(Q_OS_ANDROID)
    QString suffix = ".apk";
#else
    QString suffix = ".tar.xz";
#endif
    QByteArray arr = packReply->readAll();

    if (!packHashVerify(arr)) {
        QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nDownload pack checksum mismatch."));
#ifdef Q_OS_WIN
        taskbarButton->progress()->hide();
#endif
        QDialog::reject();
        return;
    }

    QFile file;
    file.setFileName(QString("UpdatePack") + suffix);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(arr);
    file.close();

    m_finishedPack = true;

    if (m_finishedPack && m_finishedScript)
        startUpdate();
}

void UpdateDialog::errPack()
{
#ifdef Q_OS_WIN
    taskbarButton->progress()->hide();
#endif
    if (scriptReply != NULL) {
        disconnect(scriptReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
        disconnect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
    }
    if (packReply != NULL) {
        disconnect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
        disconnect(packReply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
        disconnect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);
    }

    QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nCannot download the update pack."));
    QDialog::reject();
}

void UpdateDialog::accept()
{
}

void UpdateDialog::reject()
{
    if (!m_busy)
        QDialog::reject();
}

void UpdateDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
#ifdef Q_OS_WIN
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(windowHandle());
    QWinTaskbarProgress *prog = taskbarButton->progress();
    prog->setVisible(false);
    prog->setMinimum(0);
    prog->reset();
    prog->setMaximum(10000);
#endif
}
