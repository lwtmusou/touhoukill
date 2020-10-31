#ifndef OGG_FILE_H
#define OGG_FILE_H

#include <QBuffer>
#include <QIODevice>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

class QAudioFormat;

class OggFile final : public QIODevice{
    Q_OBJECT
public:
    explicit OggFile(QBuffer *buffer, const QString& name);
    ~OggFile();

    QAudioFormat getFormat();

protected:
    qint64 readData(char *data, qint64 max_bytes) override;
    qint64 writeData(const char *data, qint64 max_bytes) override;
    OggVorbis_File vf;
    ov_callbacks vcall;
};

#endif
