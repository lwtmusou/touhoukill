#include "audio.h"

#ifdef AUDIO_SUPPORT

#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
#include <QCache>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QThread>

#include "util.h"
#include "vorbis/vorbisfile.h"

static size_t read_from_qbuffer(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    auto res = buffer->read(size * nmemb);
    memcpy(ptr, res.data(), res.size());
    return res.size();
}

static int seek_qbuffer(void *datasource, ogg_int64_t off, int whence)
{
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    bool res = false;
    if (whence == SEEK_SET)
        res = buffer->seek(off);
    else if (whence == SEEK_CUR)
        res = buffer->seek(off + buffer->pos());
    else if (whence == SEEK_END)
        res = buffer->seek(off + buffer->size() - 1);

    return (res == true) ? 0 : 1;
}

static long tell_qbuffer(void *datasource)
{
    QBuffer *buffer = (QBuffer *)datasource;
    return buffer->pos();
}

class OggPlayer : public QObject
{
    Q_OBJECT

public:
    OggPlayer(const QString &fileName, QObject *parent = nullptr)
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

        if (ov_open_callbacks(&buffer, &vf, nullptr, 0, vcall)) {
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

        const auto vi = ov_info(&vf, -1);

        format.setChannelCount(vi->channels);
        format.setSampleRate(vi->rate);
        format.setCodec("audio/pcm");
        format.setSampleSize(16);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

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

    ~OggPlayer()
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
    AudioInternal(QObject *parent = nullptr)
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
            if (!p->isPlaying())
                p->play();

        } else {
            OggPlayer *player = new OggPlayer(fileName, this);
            soundCache.insert(fileName, player);
            player->play();
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
        if (bgmSound != nullptr)
            disconnect(bgmSound, &OggPlayer::finished, this, &AudioInternal::playNextBgm);
        delete bgmSound;
    }

    void setEffectVolume(float volume)
    {
    }

    void setBGMVolume(float volume)
    {
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
        QString f = bgmFileNames.takeFirst();
        qShuffle(bgmFileNames);
        bgmFileNames.append(f);

        playBgmInternal();
    }

private:
    void playBgmInternal()
    {
        bgmSound = new OggPlayer(bgmFileNames.first(), this);
        connect(bgmSound, &OggPlayer::finished, this, &AudioInternal::playNextBgm);
        bgmSound->play();
    }

    QStringList bgmFileNames;
    QCache<QString, OggPlayer> soundCache;
    OggPlayer *bgmSound;
};

static QThread *audioThread = nullptr;
static AudioInternal *internal = nullptr;
static bool isBgmPlaying = false;

void Audio::init()
{
    if (Q_LIKELY(audioThread == nullptr)) {
        audioThread = new QThread;
        internal = new AudioInternal;
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, []() -> void { Audio::quit(); });
        QObject::connect(audioThread, &QThread::finished, internal, &AudioInternal::deleteLater);
        internal->moveToThread(audioThread);
        audioThread->start();
        emit internal->init_S();
    }
}

void Audio::quit()
{
    if (Q_UNLIKELY(internal != nullptr)) {
        emit internal->quit_S();

        audioThread->quit();
        if (!audioThread->wait(3000))
            audioThread->terminate();

        delete audioThread;
        audioThread = nullptr;
    }
}

float Audio::volume = 1;
float Audio::bgm_volume = 1;

void Audio::play(const QString &fileName)
{
    if (Q_UNLIKELY(internal != nullptr))
        emit internal->play_S(fileName);
}

void Audio::setEffectVolume(float volume)
{
}

void Audio::setBGMVolume(float volume)
{
}

void Audio::playBGM(const QStringList &fileNames)
{
    isBgmPlaying = true;
    emit internal->playBGM_S(fileNames);
}

bool Audio::isBackgroundMusicPlaying()
{
    return isBgmPlaying;
}

void Audio::stopBGM()
{
    emit internal->stopBGM_S();
    isBgmPlaying = false;
}

QStringList Audio::getBgmFileNames(const QString fileNames, bool isGeneralName)
{
    QStringList all;
    if (fileNames.endsWith(".ogg")) {
        all = fileNames.split(";");
        return all;
    }

    if (isGeneralName) { //fileNames is  generalName
        //just support title only
        QString path = "audio/bgm/";
        QDir dir(path);
        QStringList filter;
        filter << fileNames + "_*.ogg";
        dir.setNameFilters(filter);
        QList<QFileInfo> file_info(dir.entryInfoList(filter));

        foreach (QFileInfo file, file_info) {
            QString fileName = path + file.fileName();
            if (!all.contains(fileName))
                all << fileName;
        }
    } else {
        QString path = "audio/title/";
        QDir dir(path);
        QStringList filter;
        filter << "*.ogg";
        dir.setNameFilters(filter);
        QList<QFileInfo> file_info(dir.entryInfoList(filter));

        foreach (QFileInfo file, file_info) {
            QString fileName = path + file.fileName();
            if ((file.fileName().startsWith("main") || file.fileName().startsWith("opening")))
                all << fileName;
        }
    }

    return all;
}

#include "audio.moc"

#endif // AUDIO_SUPPORT
