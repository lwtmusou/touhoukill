#ifdef AUDIO_SUPPORT

#include <QCache>
#include <QDir>
#include <QSet>
#include <QStringList>
#include <QTime>
#include <QMutex>

#include "audio.h"
#include "util.h"

class OggPlayingList{
public:
    QSet<OggPlayer *> oggs;
    QMutex mutex;
    OggPlayingList(){
        oggs.clear();
    }

    void appendToList(OggPlayer *player){
        mutex.lock();
        oggs.insert(player);
        mutex.unlock();
    }

    void removeFromList(OggPlayer *player){
        mutex.lock();
        oggs.remove(player);
        mutex.unlock();
    }

    bool contains(OggPlayer *player){
        mutex.lock();
        bool res = oggs.contains(player);
        mutex.unlock();
        return res;
    }

    void updateVolume(float volume){
        mutex.lock();
        for(auto *p : oggs){
            p->setVolume(volume);
        }
        mutex.unlock();
    }

    void stopAll(){
        mutex.lock();
        for(auto *p : oggs){
            emit p->tryToStop();
        }
        mutex.unlock();
    }

    int length(){
        mutex.lock();
        auto res = oggs.size();
        mutex.unlock();
        return res;
    }
};

static OggPlayingList bgm_list;
static OggPlayingList effective_list;

OggPlayer::OggPlayer(const QString& filename, bool is_bgm) 
    : filename(filename), repeat(false), is_bgm(is_bgm)
{
    output = nullptr;
    ogg = nullptr;
    QObject::connect(this, &OggPlayer::tryToStop, this, &OggPlayer::stop, Qt::QueuedConnection);
};

OggPlayer::~OggPlayer(){
    if(output != nullptr){
        output->stop();
        delete output;
    }
    if(ogg != nullptr){
        delete ogg;
    }
}

void OggPlayer::run(){
    if(ogg == nullptr)
        ogg = new OggFile(filename);
    
    if(output == nullptr)
        output = new QAudioOutput(ogg->getFormat());

    QObject::connect(output, &QAudioOutput::stateChanged, [&](QAudio::State s){
        if(s == QAudio::IdleState){
            output->stop();
            if(repeat){
                output->reset();
                ogg->reset();
                output->start(ogg);
            } else {
                if(is_bgm) bgm_list.removeFromList(this);
                else effective_list.removeFromList(this);
                quit();
            }
        }
    });

    if(is_bgm) {
        bgm_list.appendToList(this);
        output->setVolume(Audio::bgm_volume);
    } else {
        effective_list.appendToList(this);
        output->setVolume(Audio::volume);
    }

    output->start(ogg);
    this->exec();
}

void OggPlayer::play(bool loop){
    if(!this->isRunning()) this->start();
}

void OggPlayer::stop(){
    if(output != nullptr) output->stop();
    if(is_bgm) bgm_list.removeFromList(this);
    else effective_list.removeFromList(this);
    quit();
}

bool OggPlayer::isPlaying() const{
    return this->isRunning();
}

void OggPlayer::setVolume(float volume){
    if(output != nullptr) output->setVolume(volume);
}



class BackgroundMusicPlayList
{
public:
    enum PlayOrder
    {
        Sequential = 1,
        Shuffle = 2,
    };

    explicit BackgroundMusicPlayList(const QStringList &fileNames, BackgroundMusicPlayList::PlayOrder order = Sequential, const QStringList &openings = QStringList())
        : m_fileNames(fileNames)
        , m_order(order)
        , m_index(-1)
        , m_openings(openings)
    {
    }

    int count() const
    {
        return m_fileNames.size();
    }

    bool operator==(const BackgroundMusicPlayList &other) const
    {
        if (this == &other) {
            return true;
        }

        if (this->m_order != other.m_order) {
            return false;
        }

        switch (other.m_order) {
        case Sequential:
            return this->m_fileNames == other.m_fileNames;

        case Shuffle:
            return this->m_fileNames.toSet() == other.m_fileNames.toSet();

        default:
            return false;
        }
    }
    bool operator!=(const BackgroundMusicPlayList &other) const
    {
        return !(*this == other);
    }

    QString nextFileName()
    {
        if (Shuffle == m_order) {
            if (m_randomQueue.isEmpty()) {
                fillRandomQueue();
            }

            QString fileName = m_randomQueue.takeFirst();
            m_index = m_fileNames.indexOf(fileName);
            return fileName;
        } else {
            if (++m_index >= m_fileNames.size()) {
                m_index = 0;
            }
            return m_fileNames.at(m_index);
        }
    }

private:
    void fillRandomQueue()
    {
        m_randomQueue = m_fileNames;
        m_randomQueue.prepend("");
        qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

        if (m_openings.isEmpty())
            qShuffle(m_randomQueue);
        else {
            qShuffle(m_openings);
            QString first = m_openings.takeFirst();
            foreach (QString opening, m_openings) {
                m_randomQueue << opening;
            }
            qShuffle(m_randomQueue);
            m_randomQueue.prepend(first);
        }
    }

private:
    QStringList m_fileNames;
    QStringList m_randomQueue;
    PlayOrder m_order;
    int m_index;
    QStringList m_openings; //need play title/open at first
};

class BackgroundMusicPlayer : public QObject
{
public:
    BackgroundMusicPlayer()
        : m_timer(0)
        , m_count(0)
    {
    }

    void play(const QString &fileNames, bool random, bool playAll = false, bool isGeneralName = false)
    {
        if (m_timer != 0) {
            return;
        }

        {
            BackgroundMusicPlayList::PlayOrder playOrder = random ? BackgroundMusicPlayList::Shuffle : BackgroundMusicPlayList::Sequential;

            QStringList all;
            if (fileNames.endsWith(".ogg"))
                all = fileNames.split(";");
            QStringList openings;

            if (isGeneralName) { //fileNames is  generalName
                //just support title only
                QString path = "audio/bgm/";
                QDir *dir = new QDir(path);
                QStringList filter;
                filter << "*.ogg";
                dir->setNameFilters(filter);
                QList<QFileInfo> file_info(dir->entryInfoList(filter));

                foreach (QFileInfo file, file_info) {
                    QString fileName = path + file.fileName();
                    if (file.fileName().startsWith(fileNames + "_") && !all.contains(fileName))
                        all << fileName;
                }
            }

            if (all.isEmpty() || playAll) {
                QString path = "audio/title/";
                QDir *dir = new QDir(path);
                QStringList filter;
                filter << "*.ogg";
                dir->setNameFilters(filter);
                QList<QFileInfo> file_info(dir->entryInfoList(filter));

                foreach (QFileInfo file, file_info) {
                    QString fileName = path + file.fileName();
                    if ((file.fileName().startsWith("main") || file.fileName().startsWith("opening")))
                        openings << fileName;
                    else if (!all.contains(fileName))
                        all << fileName;
                }
            }

            QScopedPointer<BackgroundMusicPlayList> playList(new BackgroundMusicPlayList(all, playOrder, openings));
            if (!m_playList || (*m_playList != *playList)) {
                m_playList.swap(playList);
                m_count = m_playList->count();
            }
        }

        playNext();

        if (m_count >= 1) {
            m_timer = startTimer(m_interval);
        }
    }

    void stop()
    {
        if (m_timer != 0) {
            killTimer(m_timer);
            m_timer = 0;
        }
    }

    void shutdown()
    {
        m_sound.reset();
    }

protected:
    virtual void timerEvent(QTimerEvent *)
    {
        if (!m_sound->isPlaying()) {
            playNext();
        }
    }

private:
    void playNext()
    {
        m_sound.reset(new OggPlayer(m_playList->nextFileName(), true));
        m_sound->play(1 == m_count);
    }

    Q_DISABLE_COPY(BackgroundMusicPlayer)

private:
    QScopedPointer<BackgroundMusicPlayList> m_playList;
    QScopedPointer<OggPlayer> m_sound;
    int m_timer;
    int m_count;

    static const int m_interval = 500;
};

static QCache<QString, OggPlayer> SoundCache;
static BackgroundMusicPlayer backgroundMusicPlayer;
QString Audio::m_customBackgroundMusicFileName;

void Audio::init()
{
    
}

void Audio::quit()
{
    
}

float Audio::volume = 1;
float Audio::bgm_volume = 1;


void Audio::play(const QString &fileName, bool continuePlayWhenPlaying /* = false*/)
{
    OggPlayer *sound = SoundCache[fileName];
    if (NULL == sound) {
        sound = new OggPlayer(fileName);
        SoundCache.insert(fileName, sound);
    } else if (!continuePlayWhenPlaying && sound->isPlaying()) {
        return;
    }

    sound->play();
}

void Audio::setEffectVolume(float volume)
{
    Audio::volume = volume;
    effective_list.updateVolume(volume);
}

void Audio::setBGMVolume(float volume)
{
    Audio::bgm_volume = volume;
    bgm_list.updateVolume(volume);
}

void Audio::playBGM(const QString &fileNames, bool random /* = false*/, bool playAll, bool isGeneralName)
{
    if (!m_customBackgroundMusicFileName.isEmpty()) {
        backgroundMusicPlayer.play(m_customBackgroundMusicFileName, random, playAll, isGeneralName);
    } else {
        backgroundMusicPlayer.play(fileNames, random, playAll, isGeneralName);
    }
}

bool Audio::isBackgroundMusicPlaying(){
    return bgm_list.length() > 0;
}

void Audio::stopBGM()
{
    backgroundMusicPlayer.stop();
    bgm_list.stopAll();
}

void Audio::stopAll()
{
    effective_list.stopAll();
    stopBGM();

    resetCustomBackgroundMusicFileName();
}

QString Audio::getVersion()
{
    /*
     * FMOD version number.
     * 0xaaaabbcc -> aaaa = major version number.
     * bb = minor version number.
     * cc = development version number.
    */
    return "";
}

#endif // AUDIO_SUPPORT
