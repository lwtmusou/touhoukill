#ifndef AUDIO_H
#define AUDIO_H

#ifdef AUDIO_SUPPORT

#include <QString>

class Audio
{
public:
    static void init();
    static void quit();

    static void play(const QString &fileName);

    static void setEffectVolume(float volume);
    static void setBGMVolume(float volume);

    static void playBGM(const QStringList &fileNames);
    static void stopBGM();
    static bool isBackgroundMusicPlaying();

    static float volume;
    static float bgm_volume;

    static QStringList getBgmFileNames(const QString fileNames, bool isGeneralName = false);

private:
    static const int MAX_CHANNEL_COUNT = 100;
};

#endif // AUDIO_SUPPORT

#endif
