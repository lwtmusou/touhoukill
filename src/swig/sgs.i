%module sgs

%{

#include <QObject>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QByteArray>
#include <QCoreApplication>
#include <QThread>

#ifdef QT_WIDGETS_LIB
#include <QMessageBox>
#else
class QWidget;
class QMessageBox
{
public:
    static inline void warning(QWidget *parent, const QString &title, const QString &text) {}
    static inline void critical(QWidget *parent, const QString &title, const QString &text) {}
};
#endif

#include "lua-wrapper.h"

%}

%include "cryptographic.i"
%include "naturalvar.i"

class qrc {
private:
    qrc() = delete;
    ~qrc() = delete;
    qrc(const LuaQrcWrapper &) = delete;
    qrc &operator=(const LuaQrcWrapper&) = delete;
};

%extend qrc {
    static QByteArray contents(const QString &n) noexcept {
        QString fileName = n;
        if (!fileName.startsWith("qrc:"))
            return QByteArray();

        fileName = fileName.mid(3);
        QFile f(fileName);
        if (!f.exists())
            return QByteArray();

        f.open(QIODevice::ReadOnly);
        return f.readAll();
    }

    static bool contains(const QString &n) noexcept {
        QString fileName = n;
        if (!fileName.startsWith("qrc:"))
            return false;

        fileName = fileName.mid(3);
        QFileInfo f(fileName);
        return f.exists();
    }
};

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

class QWidget;

class QMessageBox {
public:
    static void warning(QWidget *parent, const QString &title, const QString &text);
    static void critical(QWidget *parent, const QString &title, const QString &text);
};

extern bool isGui();

%{
extern bool isGui();

bool isGui()
{
#ifdef QT_WIDGETS_LIB
    return QThread::currentThread() == qApp->thread();
#else
    return false;
#endif
}
%}
