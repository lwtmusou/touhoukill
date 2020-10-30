#include "OggFile.h"

#include <QAudioFormat>

OggFile::OggFile(const QString &path){
  int ret = ov_fopen(path.toUtf8(), &vf);
  if(ret < 0){
    setErrorString("Not a valid Ogg file.");
  }
  //emit this->readyRead();
  this->open(QIODevice::ReadOnly);
}

OggFile::~OggFile(){
  this->close();
  ov_clear(&vf);
}

QAudioFormat OggFile::getFormat(){
  QAudioFormat format;

  const auto vi = ov_info(&vf, -1);
  
  format.setChannelCount(vi->channels);
  format.setSampleRate(vi->rate);
  format.setCodec("audio/pcm");
  //format.setSampleType(QAudioFormat::SampleType::)
  format.setSampleSize(16);
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  return format;
}

qint64 OggFile::readData(char *data, qint64 max_byte){
  qint64 ret = ov_read(&vf, data, max_byte, 0, 2, 1, nullptr);
  if(ret < 0) setErrorString("Corrupted Ogg File");
  return ret;
}

qint64 OggFile::writeData(const char *data, qint64 max_byte){
  return 0; 
}

bool OggFile::reset(){
  return ov_time_seek(&vf, 0) == 0;
}