#ifndef AUDIO_H
#define AUDIO_H

#ifdef AUDIO_SUPPORT

#include <QAudioOutput>
#include <QString>
#include <QThread>
#include <QBuffer>

#include "OggFile.h"

class Audio
{
public:
    static void init();
    static void quit();

    static void play(const QString &fileName, bool continuePlayWhenPlaying = false);

    static void setEffectVolume(float volume);
    static void setBGMVolume(float volume);

    static void playBGM(const QString &fileNames, bool random = false, bool playAll = false, bool isGeneranlName = false);
    static void stopBGM();
    static bool isBackgroundMusicPlaying();

    static const QString &getCustomBackgroundMusicFileName()
    {
        return m_customBackgroundMusicFileName;
    }
    static void setCustomBackgroundMusicFileName(const QString &customBackgroundMusicFileName)
    {
        m_customBackgroundMusicFileName = customBackgroundMusicFileName;
    }
    static void resetCustomBackgroundMusicFileName()
    {
        m_customBackgroundMusicFileName.clear();
    }

    static void stopAll();

    static QString getVersion();

    static float volume;
    static float bgm_volume;

private:
    static QString m_customBackgroundMusicFileName;
    static const int MAX_CHANNEL_COUNT = 100;
};

class OggPlayer : public QThread
{
    Q_OBJECT
public:
    explicit OggPlayer(const QString &file_name, bool is_bgm = false);
    virtual ~OggPlayer();
    void play(bool loop = false);
    void stop();
    bool isPlaying() const;

signals:
    void setVolume(float volume);

protected:
    void run();


private:
    bool repeat;
    bool is_bgm;
    QByteArray encoding;
    QString file_name;
    bool valid;
};

#endif // AUDIO_SUPPORT

#endif
