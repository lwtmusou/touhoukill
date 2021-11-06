%{

#include <QObject>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QByteArray>

#include <QCryptographicHash>

namespace BuiltinExtension {
bool VerifyChecksum(const QString &path, const QString &hash, QCryptographicHash::Algorithm algorithm);
} // namespace BuiltinExtension
%}

class QCryptographicHash {
    enum Algorithm {
        Md4,
        Md5,
        Sha1,
        Sha224,
        Sha256,
        Sha384,
        Sha512,
        Keccak_224,
        Keccak_256,
        Keccak_384,
        Keccak_512,
        Sha3_224,
        Sha3_256,
        Sha3_384,
        Sha3_512,
    };

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(QCryptographicHash)
};

class BuiltinExtension {
public:
    static bool VerifyChecksum(const QString &path, const QString &hash, QCryptographicHash::Algorithm algorithm);

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(BuiltinExtension)
};
