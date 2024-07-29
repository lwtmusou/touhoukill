#include "updatedialog.h"
#include "engine.h"
#include "settings.h"

#include "p256.h"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
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
#include <QVersionNumber>
#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#if 0
QSgsDownloadReply::QSgsDownloadReply(QSgsDownloader *parent)
    : QObject(parent)
    , m_payloadReply(nullptr)
    , m_signatureReply(nullptr)
    , m_payloadFinished(false)
    , m_signatureFinished(false)
    , m_outputBuffer(nullptr)
{
}

QSgsDownloadReply::~QSgsDownloadReply()
{
    if (m_payloadReply != nullptr)
        m_payloadReply->deleteLater();
    if (m_signatureReply != nullptr)
        m_signatureReply->deleteLater();
}

void QSgsDownloadReply::directDownload(QNetworkAccessManager *downloader, const QString &url, QIODevice *output)
{
    QNetworkRequest reqPayload;
    reqPayload.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reqPayload.setUrl(QUrl(url));

    m_payloadFinished = false;
    delete m_payloadReply;
    m_payloadReply = downloader->get(reqPayload);
    connect(m_payloadReply, &QNetworkReply::downloadProgress, this, &QSgsDownloadReply::downloadProgress);
    connect(m_payloadReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &QSgsDownloadReply::networkErrorHandler);
    connect(m_payloadReply, &QNetworkReply::finished, this, &QSgsDownloadReply::downloadCompletedPayload);

    m_signatureFinished = true;
    delete m_signatureReply;
    m_signatureReply = nullptr;

    m_outputBuffer = output;
}

void QSgsDownloadReply::verifiedDownload(QNetworkAccessManager *downloader, const QString &url, QIODevice *output)
{
    QNetworkRequest reqPayload;
    reqPayload.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reqPayload.setUrl(QUrl(url));

    m_payloadFinished = false;
    delete m_payloadReply;
    m_payloadReply = downloader->get(reqPayload);
    connect(m_payloadReply, &QNetworkReply::downloadProgress, this, &QSgsDownloadReply::downloadProgress);
    connect(m_payloadReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &QSgsDownloadReply::networkErrorHandler);
    connect(m_payloadReply, &QNetworkReply::finished, this, &QSgsDownloadReply::downloadCompletedPayload);

    QNetworkRequest reqSignature;
    reqSignature.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reqSignature.setUrl(QUrl(url + ".sign"));

    m_signatureFinished = false;
    delete m_signatureReply;
    m_signatureReply = downloader->get(reqSignature);
    connect(m_signatureReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &QSgsDownloadReply::networkErrorHandler);
    connect(m_signatureReply, &QNetworkReply::finished, this, &QSgsDownloadReply::downloadCompleteSignature);

    m_outputBuffer = output;
}

void QSgsDownloadReply::signatureVerify()
{
    QByteArray payload = m_payloadReply->readAll();

    bool verified = false;

    if (m_signatureReply != nullptr) {
        QByteArray signature = m_signatureReply->readAll();
        bool signatureOK = false;
        if (signature.length() == 64) {
            signatureOK = true;
        } else if (signature.length() == 71) {
            // convert the 71 bytes signature to 64 bytes
            // cut-down from OpenSSL asn1parse with some assumptions (since we're building the signature file using OpenSSL)
            if (signature.at(0) == 0x30 && signature.at(1) == 0x45 && signature.at(2) == 0x02 && signature.at(3) == 0x21 && signature.at(4) == 0x00 && signature.at(37) == 0x02
                && signature.at(38) == 0x20) {
                QByteArray signature2 = signature.mid(5, 32) + signature.mid(39, 32);
                signature = signature2;
                signatureOK = true;
            }
        }

        if (signatureOK) {
            // clang-format off
            static const unsigned char publicKey[] = {
                // magic
                0x04,

                // x-coord
                0x42, 0xca, 0x7f, 0xc3, 0x83, 0xe3, 0x00, 0xea,
                0x5b, 0xd2, 0xec, 0xb3, 0x31, 0xe6, 0xf8, 0x3b,
                0x1e, 0x04, 0x10, 0x9b, 0x02, 0x05, 0xe4, 0x53,
                0x8e, 0xf1, 0x19, 0x90, 0x72, 0xc3, 0x55, 0x0d,

                // y-coord
                0x58, 0x87, 0xc8, 0xb2, 0x32, 0x08, 0xa2, 0x8a,
                0xd5, 0xf8, 0x2c, 0xc9, 0xd5, 0x9a, 0x16, 0xad,
                0x79, 0x58, 0xa4, 0xe0, 0xf0, 0x10, 0x1e, 0xf1,
                0xa8, 0x9a, 0xa8, 0x60, 0xef, 0xd4, 0x42, 0x0d,
            };
            // clang-format on
            verified = (p256_verify(reinterpret_cast<const uint8_t *>(payload.constData()), payload.length(), reinterpret_cast<const uint8_t *>(signature.constData()), publicKey)
                        == P256_SUCCESS);
        }
    } else {
        verified = true;
    }

    if (verified) {
        if (m_outputBuffer != nullptr)
            m_outputBuffer->write(payload);
        emit finished();
    } else {
        emit verificationError();
    }
}

void QSgsDownloadReply::downloadCompletedPayload()
{
    m_payloadFinished = true;
    if (m_payloadFinished && m_signatureFinished)
        signatureVerify();
}

void QSgsDownloadReply::downloadCompleteSignature()
{
    m_signatureFinished = true;
    if (m_payloadFinished && m_signatureFinished)
        signatureVerify();
}

void QSgsDownloadReply::networkErrorHandler(QNetworkReply::NetworkError e)
{
    disconnect(m_payloadReply, &QNetworkReply::downloadProgress, this, &QSgsDownloadReply::downloadProgress);
    disconnect(m_payloadReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &QSgsDownloadReply::networkErrorHandler);
    disconnect(m_payloadReply, &QNetworkReply::finished, this, &QSgsDownloadReply::downloadCompletedPayload);
    m_payloadReply->deleteLater();
    m_payloadReply = nullptr;

    if (m_signatureReply != nullptr) {
        disconnect(m_signatureReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &QSgsDownloadReply::networkErrorHandler);
        disconnect(m_signatureReply, &QNetworkReply::finished, this, &QSgsDownloadReply::downloadCompleteSignature);
        m_signatureReply->deleteLater();
        m_signatureReply = nullptr;
    }

    emit networkErrorOccurred(e);
}

QSgsDownloader::QSgsDownloader(QObject *parent)
    : QObject(parent)
{
}
#endif

// clang-format off
const uint8_t UpdateDialog::publicKey[65] = {
    // magic
    0x04,

    // x-coord
    0x42, 0xca, 0x7f, 0xc3, 0x83, 0xe3, 0x00, 0xea,
    0x5b, 0xd2, 0xec, 0xb3, 0x31, 0xe6, 0xf8, 0x3b,
    0x1e, 0x04, 0x10, 0x9b, 0x02, 0x05, 0xe4, 0x53,
    0x8e, 0xf1, 0x19, 0x90, 0x72, 0xc3, 0x55, 0x0d,

    // y-coord
    0x58, 0x87, 0xc8, 0xb2, 0x32, 0x08, 0xa2, 0x8a,
    0xd5, 0xf8, 0x2c, 0xc9, 0xd5, 0x9a, 0x16, 0xad,
    0x79, 0x58, 0xa4, 0xe0, 0xf0, 0x10, 0x1e, 0xf1,
    0xa8, 0x9a, 0xa8, 0x60, 0xef, 0xd4, 0x42, 0x0d,
};
// clang-format on

UpdateDialog::UpdateDialog(QWidget *parent)
    : QDialog(parent)
    , bar(new QProgressBar)
    , lbl(new QLabel)
    , downloadManager(new QNetworkAccessManager(this))
    , updateDescriptionFileReply(nullptr)
    , updateDescriptionFileSignatureReply(nullptr)
    , scriptReply(nullptr)
    , packReply(nullptr)
#ifdef Q_OS_WIN
    , taskbarButton(nullptr)
#endif
    , m_finishedUpdateDescriptionFile(false)
    , m_finishedUpdateDescriptionFileSignature(false)
    , m_finishedScript(false)
    , m_finishedPack(false)
    , m_busy(false)
{
    setWindowTitle(tr("Download/Update contents"));
    lbl->setVisible(false);

    connect(this, &UpdateDialog::busy, this, [this](bool busy) -> void {
        m_busy = busy;
    });

    for (int i = 0; i < UiMax; ++i) {
        currentVersion[i] = new QLabel;
        currentVersion[i]->setText(getVersionNumberForItem(static_cast<UpdateItem>(i)).toString());
        currentVersion[i]->setAlignment(Qt::AlignCenter);
        latestVersion[i] = new QLabel("X.X.X");
        latestVersion[i]->setAlignment(Qt::AlignCenter);
        updateButton[i] = new QPushButton(tr("Update"));
        // save the update item here for later use. See UpdateDialog::updateClicked
        updateButton[i]->setObjectName(QString::number(i));
        if (i == UiBase)
            updateButton[i]->setDefault(true);
        connect(updateButton[i], &QPushButton::clicked, this, &UpdateDialog::updateClicked);
        connect(this, &UpdateDialog::busy, updateButton[i], &QPushButton::setDisabled);
    }

    changelogBtn = new QPushButton(tr("Show Changelog"));
    connect(changelogBtn, &QPushButton::clicked, this, [this]() -> void {
        QDesktopServices::openUrl(QUrl(m_baseChangeLog));
    });

    bar->setMaximum(10000);

    QGridLayout *upperLayout = new QGridLayout;

    QLabel *c = new QLabel(tr("Current Version"));
    c->setAlignment(Qt::AlignCenter);
    QLabel *l = new QLabel(tr("Latest Version"));
    l->setAlignment(Qt::AlignCenter);

    upperLayout->addWidget(lbl, 0, 0, 1, 5);
    upperLayout->addWidget(c, 1, 1);
    upperLayout->addWidget(l, 1, 2);
    upperLayout->addWidget(new QLabel(tr("Base Contents")), 2, 0);
    upperLayout->addWidget(currentVersion[UiBase], 2, 1);
    upperLayout->addWidget(latestVersion[UiBase], 2, 2);
    upperLayout->addWidget(changelogBtn, 2, 3);
    upperLayout->addWidget(updateButton[UiBase], 2, 4);
    upperLayout->addWidget(new QLabel(tr("Hero Skin")), 3, 0);
    upperLayout->addWidget(currentVersion[UiSkin], 3, 1);
    upperLayout->addWidget(latestVersion[UiSkin], 3, 2);
    upperLayout->addWidget(updateButton[UiSkin], 3, 4);
    upperLayout->addWidget(new QLabel(tr("BGM")), 4, 0);
    upperLayout->addWidget(currentVersion[UiBgm], 4, 1);
    upperLayout->addWidget(latestVersion[UiBgm], 4, 2);
    upperLayout->addWidget(updateButton[UiBgm], 4, 4);

    upperLayout->addWidget(bar, 5, 0, 1, 5);

    setLayout(upperLayout);
}

void UpdateDialog::checkForUpdate()
{
    {
        QNetworkRequest req;
        req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        req.setUrl(QUrl("https://www.touhousatsu.rocks/TouhouKillUpdate0.10.json"));
        updateDescriptionFileReply = downloadManager->get(req);
        connect(updateDescriptionFileReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::updateError);
        connect(updateDescriptionFileReply, &QNetworkReply::finished, this, &UpdateDialog::updateInfoReceived);
    }
    {
        QNetworkRequest req;
        req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        req.setUrl(QUrl("https://www.touhousatsu.rocks/TouhouKillUpdate0.10.json.sig"));
        updateDescriptionFileSignatureReply = downloadManager->get(req);
        connect(updateDescriptionFileSignatureReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::updateError);
        connect(updateDescriptionFileSignatureReply, &QNetworkReply::finished, this, &UpdateDialog::updateInfoReceived);
    }
}

void UpdateDialog::updateError(QNetworkReply::NetworkError)
{
    Config.AutoUpdateDataRececived = true;
    Config.AutoUpdateNeedsRestart = true;

    disconnect(updateDescriptionFileReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, nullptr);
    disconnect(updateDescriptionFileReply, &QNetworkReply::finished, this, nullptr);
    updateDescriptionFileReply->deleteLater();
    updateDescriptionFileReply = nullptr;

    disconnect(updateDescriptionFileSignatureReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, nullptr);
    disconnect(updateDescriptionFileSignatureReply, &QNetworkReply::finished, this, nullptr);
    updateDescriptionFileSignatureReply->deleteLater();
    updateDescriptionFileSignatureReply = nullptr;
}

void UpdateDialog::updateInfoReceived()
{
    if (sender() == updateDescriptionFileReply)
        m_finishedUpdateDescriptionFile = true;
    else if (sender() == updateDescriptionFileSignatureReply)
        m_finishedUpdateDescriptionFileSignature = true;
    else
        return;

    if (!(m_finishedUpdateDescriptionFile && m_finishedUpdateDescriptionFileSignature))
        return;

    m_updateDescription = updateDescriptionFileReply->readAll();
    updateDescriptionFileReply->deleteLater();
    updateDescriptionFileReply = nullptr;

    QByteArray signature = updateDescriptionFileSignatureReply->readAll();
    updateDescriptionFileSignatureReply->deleteLater();
    updateDescriptionFileSignatureReply = nullptr;

    if (payloadVerify(m_updateDescription, signature)) {
        updateInfoVerified();
    } else {
        Config.AutoUpdateDataRececived = true;
        Config.AutoUpdateNeedsRestart = true;
    }
}

void UpdateDialog::updateInfoVerified()
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(m_updateDescription, &err);

    if (err.error != QJsonParseError::NoError) {
        Config.AutoUpdateNeedsRestart = true;
        return;
    }
    if (!doc.isObject()) {
        qDebug() << "error document when parsing update data";
        Config.AutoUpdateNeedsRestart = true;
        return;
    }

    QJsonObject fullOb = doc.object();

    do {
        QJsonObject ob;
        ob = fullOb.value("Base").toObject();
        if (!ob.contains("LatestVersion") || !ob.value("LatestVersion").isString()) {
            qDebug() << "Base LatestVersion field is incorrect";
            break;
        }

        parseVersionInfo(UiBase, ob);
    } while (false);

    do {
        QJsonObject ob;
        ob = fullOb.value("HeroSkin").toObject();
        if (!ob.contains("LatestVersion") || !ob.value("LatestVersion").isString()) {
            qDebug() << "HeroSkin LatestVersion field is incorrect";
            break;
        }

        parseVersionInfo(UiSkin, ob);
    } while (false);

    do {
        QJsonObject ob;
        ob = fullOb.value("BGM").toObject();
        if (!ob.contains("LatestVersion") || !ob.value("LatestVersion").isString()) {
            qDebug() << "BGM LatestVersion field is incorrect";
            break;
        }

        parseVersionInfo(UiBgm, ob);
    } while (false);

    for (int i = 0; i < UiMax; ++i) {
        if (m_updateContents[i].updatePack.isEmpty())
            updateButton[i]->setEnabled(false);
    }

    Config.AutoUpdateDataRececived = true;

    if (updateButton[UiBase]->isEnabled()) {
        if (m_updateContents[UiBase].updateScript.isEmpty()) {
            lbl->setText(tr("New Version %1(%3) available.<br />"
                            "But we don\'t support auto-updating from %2 to %1 on this platform.<br />"
                            "Please download the full package by clicking \"Update\" button on the \"Base Contents\" column.")
                             .arg(latestVersion[UiBase]->text())
                             .arg(getVersionNumberForItem(UiBase).toString())
                             .arg(m_baseVersionNumber));
            lbl->setTextFormat(Qt::RichText);
        } else {
            lbl->setText(tr("New Version %1(%3) available.<br />"
                            "We support auto-updating from %2 to %1 on this platform.<br />"
                            "Click \"Update\" button on the \"Base Contents\" column to update now.")
                             .arg(latestVersion[UiBase]->text())
                             .arg(getVersionNumberForItem(UiBase).toString())
                             .arg(m_baseVersionNumber));
            lbl->setTextFormat(Qt::RichText);
        }

        lbl->setVisible(true);
        exec();
    }
}

void UpdateDialog::parseVersionInfo(UpdateDialog::UpdateItem item, const QJsonObject &ob)
{
    QVersionNumber ver = QVersionNumber::fromString(ob.value("LatestVersionNumber").toString());
    latestVersion[item]->setText(ver.toString());

    if (item == UiBase) {
        if (ob.contains("ChangeLog"))
            m_baseChangeLog = ob.value("ChangeLog").toString();
        else
            changelogBtn->setVisible(false);

        m_baseVersionNumber = ob.value("LatestVersion").toString();
    }

    if (ver > getVersionNumberForItem(item)) {
        // there is a new version available now!!
        QString from = QString("From") + getVersionNumberForItem(item).toString();
        if (ob.contains(from))
            parsePackageInfo(item, ob.value(from).toObject());
        else {
            QVersionNumber pref = QVersionNumber::commonPrefix(Sanguosha->getQVersionNumber(), ver);
            from = QString("From") + pref.toString();
            if (ob.contains(from))
                parsePackageInfo(item, ob.value(from).toObject());
            else
                parsePackageInfo(item, ob.value("FullPack").toObject());
        }
    }
}

QVersionNumber UpdateDialog::getVersionNumberForItem(UpdateDialog::UpdateItem item)
{
    switch (item) {
    case UiBase:
        return Sanguosha->getQVersionNumber();
    case UiSkin: {
        QVariant v = GetConfigFromLuaState(Sanguosha->getLuaState(), "withHeroSkin");

        QString s = v.toString();
        if (s == "N/A")
            return QVersionNumber();
        else if (s.length() != 0)
            return QVersionNumber::fromString(s);

        break;
    }
    case UiBgm: {
        QVariant v = GetConfigFromLuaState(Sanguosha->getLuaState(), "withBgm");

        QString s = v.toString();
        if (s == "N/A")
            return QVersionNumber();
        else if (s.length() != 0)
            return QVersionNumber::fromString(s);

        break;
    }
    default:
        break;
    }

    return QVersionNumber();
}

void UpdateDialog::updateClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn != nullptr) {
        // previously we save the update item to objectName, so we need to extract the update item from objectName
        bool ok = false;
        UpdateItem item = static_cast<UpdateItem>(btn->objectName().toInt(&ok));
        if (ok) {
            if (m_updateContents[item].updateScript.isEmpty()) {
                QDesktopServices::openUrl(m_updateContents[item].updatePack);
            } else {
                m_updateScript = m_updateContents[item].updateScript;
                m_updateScriptSignature = m_updateContents[item].updateScriptSignature;
                m_updatePack = m_updateContents[item].updatePack;
                m_updatePackSignature = m_updateContents[item].updatePackSignature;
                startDownload();
            }
        }
    }
}

void UpdateDialog::parsePackageInfo(UpdateDialog::UpdateItem item, const QJsonObject &ob)
{
#if defined(Q_OS_WIN)
    QJsonValue value = ob.value("Win");
#elif defined(Q_OS_ANDROID)
    QJsonValue value = ob.value("And");
#elif defined(Q_OS_MACOS)
    QJsonValue value = ob.value("Mac");
#else
    QJsonValue value = ob.value("Oth");
#endif
    if (value.isString()) {
        m_updateContents[item].updatePack = value.toString();
    } else if (value.isObject()) {
        QJsonObject updateOb = value.toObject();
        QString updateScript = updateOb.value("UpdateScript").toString();
        QString updateScriptSignature = updateOb.value("UpdateScriptSignature").toString();
        QString updatePack = updateOb.value("UpdatePack").toString();
        QString updatePackSignature = updateOb.value("UpdatePackSignature").toString();
        if (!updateScript.isEmpty() && !updateScriptSignature.isEmpty() && !updatePack.isEmpty() && !updatePackSignature.isEmpty()) {
            m_updateContents[item].updateScript = updateScript;
            m_updateContents[item].updateScriptSignature = updateScriptSignature;
            m_updateContents[item].updatePack = updatePack;
            m_updateContents[item].updatePackSignature = updatePackSignature;
        }
    }
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
#else
#ifdef Q_OS_ANDROID
    if (m_updateScript == "jni") {
        // call jni
    } else {
#endif
        QStringList arg;
        arg << "-c" << ("\"./UpdateScript.sh " + QString::number(QCoreApplication::applicationPid()) + "\"");
        QProcess::startDetached("sh", arg, QCoreApplication::applicationDirPath());
#ifdef Q_OS_ANDROID
    }
#endif
#endif

    QCoreApplication::exit(0);
    QDialog::accept();
}

bool UpdateDialog::payloadVerify(const QByteArray &payload, const QByteArray &signature_)
{
    bool signatureOK = false;
    bool verified = false;

    QByteArray signature = signature_;

    if (signature.length() == 64) {
        signatureOK = true;
    } else if (signature.length() == 71) {
        // convert the 71 bytes signature to 64 bytes
        // cut-down from OpenSSL asn1parse with some assumptions (since we're building the signature file using OpenSSL)
        if (signature.at(0) == 0x30 && signature.at(1) == 0x45 && signature.at(2) == 0x02 && signature.at(3) == 0x21 && signature.at(4) == 0x00 && signature.at(37) == 0x02
            && signature.at(38) == 0x20) {
            QByteArray signature2 = signature.mid(5, 32) + signature.mid(39, 32);
            signature = signature2;
            signatureOK = true;
        }
    }

    if (signatureOK)
        verified = (p256_verify(reinterpret_cast<const uint8_t *>(payload.constData()), payload.length(), reinterpret_cast<const uint8_t *>(signature.constData()), publicKey)
                    == P256_SUCCESS);

    return verified;
}

bool UpdateDialog::payloadVerify(const QByteArray &payload, const QString &signature)
{
    // signature as a string is encoded as hexidecimal integers, so use fromHex
    return payloadVerify(payload, QByteArray::fromHex(signature.toLatin1()));
}

void UpdateDialog::startDownload()
{
    if (m_updatePack.isEmpty() || m_updateScript.isEmpty()) {
        QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nURL is empty."));
        return;
    }

    emit busy(true);

    QNetworkRequest reqPack;
    reqPack.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reqPack.setUrl(QUrl(m_updatePack));
    packReply = downloadManager->get(reqPack);
    connect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
    connect(packReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
    connect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);

#ifdef Q_OS_ANDROID
    if (m_updateScript != "jni") {
#endif
        QNetworkRequest reqScript;
        reqScript.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        reqScript.setUrl(QUrl(m_updateScript));
        scriptReply = downloadManager->get(reqScript);
        connect(scriptReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
        connect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
#ifdef Q_OS_ANDROID
    } else
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

    if (!payloadVerify(arr, m_updateScriptSignature)) {
        Config.AutoUpdateNeedsRestart = true;
        QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nDownload script signature mismatch."));
#ifdef Q_OS_WIN
        taskbarButton->progress()->hide();
#endif
        QDialog::reject();
        return;
    }

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
    if (scriptReply != nullptr) {
        disconnect(scriptReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
        disconnect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
    }
    if (packReply != nullptr) {
        disconnect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
        disconnect(packReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
        disconnect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);
    }

    Config.AutoUpdateNeedsRestart = true;
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

    if (!payloadVerify(arr, m_updatePackSignature)) {
        Config.AutoUpdateNeedsRestart = true;
        QMessageBox::critical(this, tr("Update Error"), tr("An error occurred when downloading packages.\nDownload pack signature mismatch."));
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
    if (scriptReply != nullptr) {
        disconnect(scriptReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errScript);
        disconnect(scriptReply, &QNetworkReply::finished, this, &UpdateDialog::finishedScript);
    }
    if (packReply != nullptr) {
        disconnect(packReply, &QNetworkReply::downloadProgress, this, &UpdateDialog::downloadProgress);
        disconnect(packReply, (void(QNetworkReply::*)(QNetworkReply::NetworkError))(&QNetworkReply::error), this, &UpdateDialog::errPack);
        disconnect(packReply, &QNetworkReply::finished, this, &UpdateDialog::finishedPack);
    }

    Config.AutoUpdateNeedsRestart = true;
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
