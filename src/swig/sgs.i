%module sgs

%{
#include "global.h"
#include "engine.h"
#include "lua-wrapper.h"
#include "card.h"
#include "CardFace.h"
#include "structs.h"
#include "player.h"
#include "general.h"

#include <QObject>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QByteArray>
#include <QCoreApplication>
#include <QThread>

// A generic way for pop Lua stack when function returns
namespace {
    class LuaDelayedPop final {
    public:
        LuaDelayedPop(lua_State *l, int n = 1) : l(l), n(1) {}
        ~LuaDelayedPop() { lua_pop(l, n); }
    private:
        lua_State *const l;
        int n;
    };
}

%}

#define Q_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define QSGS_DISABLE_COPY_MOVE_CONSTRUCT(Class) \
    Q_DISABLE_COPY_MOVE(Class) \
    Class() = delete; \
    ~Class() = delete;

// need to include this first!!
%include "naturalvar.i"

%include "cryptographic.i"
%include "list.i"

class qrc {
private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(qrc)
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

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(QObject)
};

%include "sgs_core.i"

%include "wrap_cardface.i"
