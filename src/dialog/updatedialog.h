#include <QDialog>
#include <QJsonObject>

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
