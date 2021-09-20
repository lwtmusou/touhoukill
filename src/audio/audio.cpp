#include "audio.h"

#ifdef AUDIO_SUPPORT

#include <QAudioFormat>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
#else
#include <QAudioOutput>
#endif

#include <QBuffer>
#include <QByteArray>
#include <QCache>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QThread>

#include <vorbis/vorbisfile.h>

#include "settings.h"
#include "util.h"

namespace {
size_t read_from_qbuffer(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    auto res = buffer->read(size * nmemb);
    memcpy(ptr, res.data(), res.size());
    return res.size();
}

int seek_qbuffer(void *datasource, ogg_int64_t off, int whence)
{
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    bool res = false;
    if (whence == SEEK_SET)
        res = buffer->seek(off);
    else if (whence == SEEK_CUR)
        res = buffer->seek(off + buffer->pos());
    else if (whence == SEEK_END)
        res = buffer->seek(off + buffer->size() - 1);

    return (res) ? 0 : 1;
}

long tell_qbuffer(void *datasource)
{
    QBuffer *buffer = (QBuffer *)datasource;
    return buffer->pos();
}
} // namespace

class OggPlayer : public QObject
{
    Q_OBJECT

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    typedef ::QAudioSink QAudioOutput;
#endif

public:
    explicit OggPlayer(const QString &fileName, QObject *parent = nullptr)
        : QObject(parent)
        , output(nullptr)
    {
        QFile xf(fileName);
        xf.open(QFile::ReadOnly);

        if (!xf.isOpen())
            return;

        QByteArray arr = xf.readAll();
        xf.close();

        QBuffer buffer(&arr);
        buffer.open(QIODevice::ReadOnly);
        buffer.seek(0);

        soundBuffer.open(QIODevice::WriteOnly);

        OggVorbis_File vf;
        ov_callbacks vcall;
        vcall.read_func = &read_from_qbuffer;
        vcall.seek_func = &seek_qbuffer;
        vcall.close_func = nullptr;
        vcall.tell_func = &tell_qbuffer;

        if (ov_open_callbacks(&buffer, &vf, nullptr, 0, vcall) != 0) {
            return;
        } else {
            qint64 ret = 1;
            while (ret > 0) {
                char data[20000];
                ret = ov_read(&vf, data, 20000, 0, 2, 1, nullptr);
                if (ret > 0) {
                    soundBuffer.write(data, ret);
                } else if (ret == OV_HOLE) {
                    ret = 1;
                } else if (ret == OV_EBADLINK) {
                    ret = 1;
                } else if (ret == OV_EINVAL) {
                    ret = 1;
                }
            }
        }

        const auto *vi = ov_info(&vf, -1);

        format.setChannelCount(vi->channels);
        format.setSampleRate(vi->rate);

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        format.setSampleFormat(QAudioFormat::Int16);
#else
        format.setCodec(QStringLiteral("audio/pcm"));
        format.setSampleSize(16);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);
#endif

        ov_clear(&vf);
        buffer.close();

        soundBuffer.close();
        soundBuffer.open(QIODevice::ReadOnly);

        output = new QAudioOutput(format);

        connect(output, &QAudioOutput::stateChanged, [&](QAudio::State s) {
            if (s == QAudio::IdleState) {
                emit finished();
            }
        });
    }

    ~OggPlayer() override
    {
        stop();
    }

    bool isPlaying()
    {
        if (output != nullptr)
            return output->state() == QAudio::ActiveState;

        return false;
    }

    void play()
    {
        soundBuffer.reset();
        if (output != nullptr)
            output->start(&soundBuffer);
    }

    void stop()
    {
        if (output != nullptr)
            output->stop();
    }

    void setVolume(float volume)
    {
        if (output != nullptr)
            output->setVolume(volume);
    }

signals:
    void finished();

private:
    QBuffer soundBuffer;
    QAudioFormat format;
    QAudioOutput *output;
};

class AudioInternal : public QObject
{
    Q_OBJECT

public:
    explicit AudioInternal(QObject *parent = nullptr)
        : QObject(parent)
        , bgmSound(nullptr)
    {
        connect(this, &AudioInternal::init_S, this, &AudioInternal::init, Qt::QueuedConnection);
        connect(this, &AudioInternal::quit_S, this, &AudioInternal::quit, Qt::QueuedConnection);
        connect(this, &AudioInternal::playBGM_S, this, &AudioInternal::playBGM, Qt::QueuedConnection);
        connect(this, &AudioInternal::stopBGM_S, this, &AudioInternal::stopBGM, Qt::QueuedConnection);
        connect(this, &AudioInternal::play_S, this, &AudioInternal::play, Qt::QueuedConnection);
        connect(this, &AudioInternal::setEffectVolume_S, this, &AudioInternal::setEffectVolume, Qt::QueuedConnection);
        connect(this, &AudioInternal::setBGMVolume_S, this, &AudioInternal::setBGMVolume, Qt::QueuedConnection);
    }

public slots:
    void init()
    {
        bgm_volume = 1.0;
        effective_volume = 1.0;
    }

    void quit()
    {
        delete bgmSound;
        soundCache.clear();
    }

    void play(const QString &fileName)
    {
        if (soundCache.contains(fileName)) {
            OggPlayer *p = soundCache.object(fileName);
            p->setVolume(effective_volume);
            if (!p->isPlaying())
                p->play();

        } else {
            OggPlayer *player = new OggPlayer(fileName, this);
            if (soundCache.insert(fileName, player)) {
                player->setVolume(effective_volume);
                player->play();
            }
        }
    }

    void playBGM(const QStringList &fileNames)
    {
        stopBGM();

        bgmFileNames = fileNames;
        qShuffle(bgmFileNames);

        playBgmInternal();
    }

    void stopBGM()
    {
        if (bgmSound != nullptr) {
            disconnect(bgmSound, &OggPlayer::finished, this, &AudioInternal::playNextBgm);
            bgmSound->stop();
        }

        delete bgmSound;
        bgmSound = nullptr;
    }

    void setEffectVolume(float volume)
    {
        this->effective_volume = QAudio::convertVolume(volume, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
    }

    void setBGMVolume(float volume)
    {
        this->bgm_volume = QAudio::convertVolume(volume, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
        if (bgmSound != nullptr)
            bgmSound->setVolume(this->bgm_volume);
    }

signals:
    void init_S();
    void quit_S();

    void play_S(const QString &fileName);

    void playBGM_S(const QStringList &fileNames);
    void stopBGM_S();

    void setEffectVolume_S(float volume);
    void setBGMVolume_S(float volume);

private slots:
    void playNextBgm()
    {
        if (bgmFileNames.empty())
            return;
        QString f = bgmFileNames.takeFirst();
        qShuffle(bgmFileNames);
        bgmFileNames.append(f);

        playBgmInternal();
    }

private:
    void playBgmInternal()
    {
        if (bgmFileNames.empty())
            return;
        bgmSound = new OggPlayer(bgmFileNames.first(), this);
        bgmSound->setVolume(bgm_volume);
        connect(bgmSound, &OggPlayer::finished, this, &AudioInternal::playNextBgm);
        bgmSound->play();
    }

    QStringList bgmFileNames;
    QCache<QString, OggPlayer> soundCache;
    OggPlayer *bgmSound;

    float bgm_volume;
    float effective_volume;
};

#endif // AUDIO_SUPPORT

namespace Audio {
namespace {
#ifdef AUDIO_SUPPORT
QThread *audioThread = nullptr;
AudioInternal *internal = nullptr;
#endif
bool isBgmPlaying = false;
} // namespace

void init()
{
#ifdef AUDIO_SUPPORT
    if (Q_LIKELY(audioThread == nullptr)) {
        audioThread = new QThread;
        internal = new AudioInternal;
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, []() -> void {
            Audio::quit();
        });
        QObject::connect(audioThread, &QThread::finished, []() {
            internal->deleteLater();
            internal = nullptr;
        });
        internal->moveToThread(audioThread);
        audioThread->start();
        emit internal->init_S();
    }
#endif
}

void quit()
{
#ifdef AUDIO_SUPPORT
    if (Q_LIKELY(internal != nullptr)) {
        emit internal->quit_S();

        audioThread->quit();
        if (!audioThread->wait(3000))
            audioThread->terminate();

        delete audioThread;
        audioThread = nullptr;
    }
#endif
}

float volume = 1;
float bgm_volume = 1;

void play(const QString &fileName)
{
#ifdef AUDIO_SUPPORT
    if (Q_LIKELY(internal != nullptr))
        emit internal->play_S(fileName);
#endif
}

void setEffectVolume(float volume)
{
#ifdef AUDIO_SUPPORT
    if (Q_LIKELY(internal != nullptr))
        emit internal->setEffectVolume_S(volume);
#endif
}

void setBGMVolume(float volume)
{
#ifdef AUDIO_SUPPORT
    if (Q_LIKELY(internal != nullptr))
        emit internal->setBGMVolume_S(volume);
#endif
}

void playBGM(const QStringList &fileNames)
{
#ifdef AUDIO_SUPPORT
    isBgmPlaying = true;
    emit internal->playBGM_S(fileNames);
#endif
}

bool isBackgroundMusicPlaying()
{
    return isBgmPlaying;
}

void stopBGM()
{
#ifdef AUDIO_SUPPORT
    emit internal->stopBGM_S();
    isBgmPlaying = false;
#endif
}

QStringList getBgmFileNames(const QString &fileNames, bool isGeneralName)
{
    QStringList all;
    if (fileNames.endsWith(QStringLiteral(".ogg"))) {
        all = fileNames.split(QStringLiteral(";"));
        return all;
    }

    if (isGeneralName) { //fileNames is  generalName
        //just support title only
        QString path = QStringLiteral("audio/bgm/");
        QDir dir(path);
        QStringList filter;
        filter << fileNames + QStringLiteral("_*.ogg");
        dir.setNameFilters(filter);
        QList<QFileInfo> file_info(dir.entryInfoList(filter));

        foreach (QFileInfo file, file_info) {
            QString fileName = path + file.fileName();
            if (!all.contains(fileName))
                all << fileName;
        }
    } else {
        QString path = QStringLiteral("audio/title/");
        QDir dir(path);
        QStringList filter;
        filter << QStringLiteral("*.ogg");
        dir.setNameFilters(filter);
        QList<QFileInfo> file_info(dir.entryInfoList(filter));

        foreach (QFileInfo file, file_info) {
            QString fileName = path + file.fileName();
            if ((file.fileName().startsWith(QStringLiteral("main")) || file.fileName().startsWith(QStringLiteral("opening"))))
                all << fileName;
        }
    }

    return all;
}

void playSystemAudioEffect(const QString &name)
{
    playAudioEffect(QStringLiteral("audio/system/%1.ogg").arg(name));
}

void playAudioEffect(const QString &filename)
{
#ifdef AUDIO_SUPPORT
    if (!Config.EnableEffects)
        return;
    if (filename.isNull())
        return;

    Audio::play(filename);
#endif
}

void playSkillAudioEffect(const QString & /*unused*/, int /*unused*/)
{
    // TODO: move this function to UI
#if 0
    const Skill *skill = skills.value(skill_name, NULL);
    if (skill)
        skill->playAudioEffect(index);
#endif
}

void GeneralLastWord(const QString &generalName)
{
    QString filename = QStringLiteral("audio/death/%1.ogg").arg(generalName);
    bool fileExists = QFile::exists(filename);
    if (!fileExists) {
        QStringList origin_generals = generalName.split(QStringLiteral("_"));
        if (origin_generals.length() > 1)
            filename = QStringLiteral("audio/death/%1.ogg").arg(origin_generals.first());
    }
    Audio::playAudioEffect(filename);
}

} // namespace Audio

#ifdef AUDIO_SUPPORT
#include "audio.moc"
#endif
