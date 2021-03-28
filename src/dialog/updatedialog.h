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

class UpdateDialog : public QDialog
{
    Q_OBJECT

private:
    struct UpdateContents
    {
        QString updateScript;
        QString updatePack;
        QJsonObject updateHash;
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
    void setInfo(const QString &v, const QVersionNumber &vn, const QString &updatePackOrAddress, const QJsonObject &updateHash, const QString &updateScript);

private:
    QProgressBar *bar;
    QLabel *lbl;
    QNetworkAccessManager *downloadManager;
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

    bool m_configLuaIs097;

    QString m_updateScript;
    QString m_updatePack;
    QJsonObject m_updateHash;

    bool m_finishedScript;
    bool m_finishedPack;

    bool m_busy;

    void startUpdate();
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

public slots:
    void accept() override;
    void reject() override;

    void checkForUpdate();

protected:
    void showEvent(QShowEvent *event) override;
};

#endif
