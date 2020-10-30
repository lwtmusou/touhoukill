#ifndef OGG_FILE_H
#define OGG_FILE_H

#include <QIODevice>
#include <QString>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

class QAudioFormat;

class OggFile final : public QIODevice {
  Q_OBJECT
public:
  explicit OggFile(const QString &path);
  virtual ~OggFile();

  qint64 readData(char *data, qint64 max_bytes) override;
  qint64 writeData(const char *data, qint64 max_bytes) override;

  QAudioFormat getFormat();

private:
  OggVorbis_File vf;
  int current_section;
};

#endif