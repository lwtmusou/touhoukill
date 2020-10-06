#ifndef UPDATE_DIALOG_H_
#define UPDATE_DIALOG_H_

#include <QDialog>
#include <QJsonObject>
#include <QNetworkReply>

class QWidget;
class QString;
#if QT_VERSION > 0x050600
class QVersionNumber;
#endif
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

public:
    explicit UpdateDialog(QWidget *parent = 0);
#if QT_VERSION >= 0x050600
    void setInfo(const QString &v, const QVersionNumber &vn, const QString &updatePackOrAddress, const QJsonObject &updateHash, const QString &updateScript);
#else
    void setInfo(const QString &v, const QString &vn, const QString &updatePackOrAddress, const QJsonObject &updateHash, const QString &updateScript);
#endif

private:
    QProgressBar *bar;
    QLabel *lbl;
    QNetworkAccessManager *downloadManager;
    QNetworkReply *scriptReply;
    QNetworkReply *packReply;
#ifdef Q_OS_WIN
    QWinTaskbarButton *taskbarButton;
#endif

    QString m_updateScript;
    QString m_updatePack;
    QJsonObject m_updateHash;

    bool m_finishedScript;
    bool m_finishedPack;

    bool m_busy;

    void startUpdate();
    bool packHashVerify(const QByteArray &arr);

#if QT_VERSION >= 0x050600
    void parseUpdateInfo(const QString &v, const QVersionNumber &vn, const QJsonObject &ob);
#else
    void parseUpdateInfo(const QString &v, const QString &vn, const QJsonObject &ob);
#endif

private slots:
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
