#ifndef OGG_FILE_H
#define OGG_FILE_H

#include <QIODevice>
#include <QString>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

class QAudioFormat;

class OggFile final : public QIODevice
{
    Q_OBJECT

public:
    explicit OggFile(const QString &path);
    ~OggFile();

    bool reset() override;

    QAudioFormat getFormat();

protected:
    qint64 readData(char *data, qint64 max_bytes) override;
    qint64 writeData(const char *data, qint64 max_bytes) override;

private:
    OggVorbis_File vf;
};

#endif
