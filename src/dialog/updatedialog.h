#ifndef UPDATE_DIALOG_H_
#define UPDATE_DIALOG_H_

#include <QDialog>
#include <QJsonObject>
#include <QNetworkReply>

class QWidget;
class QString;
class QVersionNumber;
class QProgressBar;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
#ifdef Q_OS_WIN
class QWinTaskbarButton;
#endif
class QShowEvent;

#if 0
class QSgsDownloader;

class QSgsDownloadReply : public QObject
{
    Q_OBJECT

    friend class QSgsDownloader;

private:
    QSgsDownloadReply(QSgsDownloader *parent);

public:
    ~QSgsDownloadReply() override;

private:
    void directDownload(QNetworkAccessManager *downloader, const QString &url, QIODevice *output);
    void verifiedDownload(QNetworkAccessManager *downloader, const QString &url, QIODevice *output);
    void signatureVerify();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void networkErrorOccurred(QNetworkReply::NetworkError);
    void finished();
    void verificationError();

private:
    QNetworkReply *m_payloadReply;
    QNetworkReply *m_signatureReply;

    bool m_payloadFinished;
    bool m_signatureFinished;

    QIODevice *m_outputBuffer;

private slots:
    void downloadCompletedPayload();
    void downloadCompleteSignature();

    void networkErrorHandler(QNetworkReply::NetworkError);
};

class QSgsDownloader : public QObject
{
    Q_OBJECT

public:
    QSgsDownloader(QObject *parent = nullptr);

    QSgsDownloadReply *verifiedDownload(const QString &url);

private:
    QNetworkAccessManager *downloadManager;
};
#endif
class UpdateDialog : public QDialog
{
    Q_OBJECT

private:
    struct UpdateContents
    {
        QString updateScript;
        QString updateScriptSignature;
        QString updatePack;
        QString updatePackSignature;
    };

    enum UpdateItem
    {
        UiBase,
        UiSkin,
        UiBgm,

        UiMax,
    };

public:
    explicit UpdateDialog(QWidget *parent = nullptr);

private:
    QProgressBar *bar;
    QLabel *lbl;
    QNetworkAccessManager *downloadManager;
    QNetworkReply *updateDescriptionFileReply;
    QNetworkReply *updateDescriptionFileSignatureReply;
    QNetworkReply *scriptReply;
    QNetworkReply *packReply;
    QPushButton *changelogBtn;

    QLabel *currentVersion[UiMax];
    QLabel *latestVersion[UiMax];
    QPushButton *updateButton[UiMax];

#ifdef Q_OS_WIN
    QWinTaskbarButton *taskbarButton;
#endif

    UpdateContents m_updateContents[UiMax];
    QString m_baseChangeLog;
    QString m_baseVersionNumber;

    QString m_updateScript;
    QString m_updateScriptSignature;
    QString m_updatePack;
    QString m_updatePackSignature;

    bool m_finishedUpdateDescriptionFile;
    bool m_finishedUpdateDescriptionFileSignature;

    bool m_finishedScript;
    bool m_finishedPack;

    bool m_busy;

    QByteArray m_updateDescription;

    void startUpdate();
    bool payloadVerify(const QByteArray &payload, const QByteArray &signature);
    bool payloadVerify(const QByteArray &payload, const QString &signature);
    bool packHashVerify(const QByteArray &arr);

    void parseVersionInfo(UpdateItem item, const QJsonObject &ob);
    void parsePackageInfo(UpdateItem item, const QJsonObject &ob);
    QVersionNumber getVersionNumberForItem(UpdateItem item);

signals:
    void busy(bool);

private slots:
    void updateClicked();
    void startDownload();
    void downloadProgress(quint64 downloaded, quint64 total);
    void finishedScript();
    void errScript();
    void finishedPack();
    void errPack();

    void updateError(QNetworkReply::NetworkError e);
    void updateInfoReceived();
    void updateInfoVerified();

public slots:
    void accept() override;
    void reject() override;

    void checkForUpdate();

protected:
    void showEvent(QShowEvent *event) override;

private:
    static const uint8_t publicKey[65];
};

#endif
