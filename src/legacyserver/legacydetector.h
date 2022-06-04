#ifndef qsgslegacy_DETECTOR_H
#define qsgslegacy_DETECTOR_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QUdpSocket>

class LegacyDetector : public QObject
{
    Q_OBJECT

public:
    LegacyDetector();
    void detect();
    void stop();

private slots:
    void onReadReady();

signals:
    void detected(const QString &server_name, const QString &address);

private:
    QUdpSocket *socket;
};

#endif
