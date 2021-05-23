#ifndef AUDIO_H
#define AUDIO_H

#ifdef AUDIO_SUPPORT

#include <QString>

namespace Audio {
void init();
void quit();

void play(const QString &fileName);

void setEffectVolume(float volume);
void setBGMVolume(float volume);

void playBGM(const QStringList &fileNames);
void stopBGM();
bool isBackgroundMusicPlaying();

extern float volume;
extern float bgm_volume;

QStringList getBgmFileNames(const QString fileNames, bool isGeneralName = false);

}

#endif // AUDIO_SUPPORT

#endif
