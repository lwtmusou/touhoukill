#ifndef _DETECTOR_H
#define _DETECTOR_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QUdpSocket>

class Detector : public QObject
{
    Q_OBJECT

public slots:
    virtual void detect() = 0;
    virtual void stop() = 0;

signals:
    void detected(const QString &server_name, const QString &address);
};

class UdpDetector : public Detector
{
    Q_OBJECT

public:
    UdpDetector();
    void detect() override;
    void stop() override;

private slots:
    void onReadReady();

private:
    QUdpSocket *socket;
};

#endif
