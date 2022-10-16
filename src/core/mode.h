#ifndef TOUHOUKILL_MODE_H
#define TOUHOUKILL_MODE_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"
#include <QSet>
#include <QString>
#endif

class Rule;
class Package;
class RoomObject;
class General;
class GameLogic;

#ifndef SWIG
class ModePrivate;
#endif

class QSGS_CORE_EXPORT Mode
{
protected:
#ifndef SWIG
    explicit Mode(const QString &name, QSanguosha::ModeCategory category);
#endif

public:
#ifndef SWIG
    virtual ~Mode();
#endif

    const QString &name() const;
    QSanguosha::ModeCategory category() const;

    virtual int playersCount() const = 0;
    virtual int generalsPerPlayer() const = 0;
    virtual QString roles() const = 0;
    virtual Rule *rule() const = 0;
    virtual IdSet availableCards() const;
    virtual QSet<const General *> availableGenerals() const;

    // TODO: Should this be here?
    // since only Server is running game logic, put this in server maybe a good idea?
    virtual void startGame(GameLogic *logic, RoomObject *room) const = 0;

private:
    ModePrivate *const d;
    Q_DISABLE_COPY_MOVE(Mode)
#ifdef SWIG
    Mode() = delete;
#endif
};

// I don't think that implementing these modes here is a good idea.
// But there seems to be no other place for them for now

#ifndef SWIG
class GenericRoleModePrivate;

class QSGS_CORE_EXPORT GenericRoleMode : public Mode
{
    explicit GenericRoleMode(const QString &name);
    static bool nameMatched(const QString &name);

public:
    ~GenericRoleMode() override;

    int playersCount() const override;
    int generalsPerPlayer() const override;
    QString roles() const override;
    Rule *rule() const override;

    void startGame(GameLogic *logic, RoomObject *room) const override;

private:
    friend class Engine;
    GenericRoleModePrivate *const d;
};

class GenericHegemonyModePrivate;

class QSGS_CORE_EXPORT GenericHegemonyMode : public Mode
{
    explicit GenericHegemonyMode(const QString &name);
    static bool nameMatched(const QString &name);

public:
    ~GenericHegemonyMode() override;

    int playersCount() const override;
    int generalsPerPlayer() const override;
    QString roles() const override;
    Rule *rule() const override;

    void startGame(GameLogic *logic, RoomObject *room) const override;

private:
    friend class Engine;
    GenericHegemonyModePrivate *const d;
};
#endif

#endif
