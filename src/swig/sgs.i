%module sgs

%{

#include <QObject>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QByteArray>

typedef void *LuaQrcWrapper;
LuaQrcWrapper qrc = nullptr;

%}

%include "naturalvar.i"

class LuaQrcWrapper {
private:
    LuaQrcWrapper() = delete;
    ~LuaQrcWrapper() = delete;
    LuaQrcWrapper(const LuaQrcWrapper &) = delete;
    LuaQrcWrapper &operator=(const LuaQrcWrapper&) = delete;
};

%extend LuaQrcWrapper {
    QByteArray contents(const QString &n) noexcept {
        QString fileName = n;
        if (!fileName.startsWith("qrc:"))
            return QByteArray();

        fileName.chop(3);
        QFile f(fileName);
        if (!f.exists())
            return QByteArray();

        f.open(QIODevice::ReadOnly);
        return f.readAll();
    }

    bool contains(const QString &n) noexcept {
        QString fileName = n;
        if (!fileName.startsWith("qrc:"))
            return false;

        fileName = fileName.mid(3);
        QFileInfo f(fileName);
        return f.exists();
    }
};

extern LuaQrcWrapper qrc;

class QObject {
public:
    QString objectName();
    void setObjectName(const char *name);
    bool inherits(const char *class_name);
    bool setProperty(const char *name, const QVariant &value);
    QVariant property(const char *name) const;
    void setParent(QObject *parent);
    void deleteLater();
};
