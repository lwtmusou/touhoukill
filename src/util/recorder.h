#ifndef _RECORDER_H
#define _RECORDER_H

#include "protocol.h"

#include <QImage>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QSemaphore>
#include <QThread>
#include <QTime>

class Recorder : public QObject
{
    Q_OBJECT

public:
    explicit Recorder(QObject *parent = nullptr);

    static QImage TXT2PNG(QByteArray data);
    static QByteArray PNG2TXT(const QString &filename);

    bool save(const QString &filename) const;
    void recordLine(const QString &line);
    QList<QByteArray> getRecords() const;

public slots:
    void record(const char *line);

private:
    QElapsedTimer watch;
    QByteArray data;
};

class Replayer : public QThread
{
    Q_OBJECT

public:
    explicit Replayer(QObject *parent, const QString &filename);

    int getDuration() const;
    qreal getSpeed();

    QString getPath() const;

    int m_commandSeriesCounter;

public slots:
    void uniform();
    void toggle();
    void speedUp();
    void slowDown();

protected:
    void run() override;

private:
    QString filename;
    qreal speed;
    bool playing;
    QMutex mutex;
    QSemaphore play_sem;

    struct Pair
    {
        int elapsed;
        QString cmd;
    };
    QList<Pair> pairs;

signals:
    void command_parsed(const QString &cmd);
    void elasped(int secs);
    void speed_changed(qreal speed);
};

#endif
