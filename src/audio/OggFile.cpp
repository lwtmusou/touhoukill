#include "OggFile.h"

#include <QAudioFormat>

QAudioFormat OggFile::getFormat()
{
    QAudioFormat format;

    const auto vi = ov_info(&vf, -1);

    format.setChannelCount(vi->channels);
    format.setSampleRate(vi->rate);
    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    return format;
}

static size_t read_from_qbuffer(void *ptr, size_t size, size_t nmemb, void *datasource){
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    auto res = buffer->read(size * nmemb);
    memcpy(ptr, res.data(), res.size());
    return res.size();
}

static int seek_qbuffer(void *datasource, ogg_int64_t off, int whence){
    QBuffer *buffer = reinterpret_cast<QBuffer *>(datasource);
    bool res = false;
    if(whence == SEEK_SET)
        res = buffer->seek(off);
    else if(whence == SEEK_CUR)
        res = buffer->seek(off + buffer->pos());
    else if(whence == SEEK_END)
        res = buffer->seek(off + buffer->size() - 1);

    return (res == true) ? 0 : 1;
}

static long tell_qbuffer(void *datasource){
    QBuffer *buffer = (QBuffer *)datasource;
    return buffer->pos();
}

OggFile::OggFile(QBuffer *buffer){
    vcall.read_func = &read_from_qbuffer;
    vcall.seek_func = &seek_qbuffer;
    vcall.close_func = nullptr;
    vcall.tell_func = &tell_qbuffer;

    if(ov_open_callbacks(buffer, &this->vf, nullptr, 0, vcall)){
        printf("Not a valid Ogg file.\n");
    } else {
        open(QIODevice::ReadOnly);
    }
}

OggFile::~OggFile(){
    if(isOpen()){
        close();
        ov_clear(&vf);
    }
}

qint64 OggFile::readData(char *data, qint64 max_byte)
{
    qint64 ret = ov_read(&vf, data, max_byte, 0, 2, 1, nullptr);
    if (ret < 0)
        setErrorString("Corrupted Ogg File");
    return ret;
}

qint64 OggFile::writeData(const char *, qint64)
{
    Q_UNREACHABLE();
    return 0;
}
