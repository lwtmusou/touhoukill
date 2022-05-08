#ifndef TOUHOUKILL_MODE_H
#define TOUHOUKILL_MODE_H

#include "global.h"

#include <QSet>
#include <QString>

class Rule;
class Package;
class RoomObject;
class General;

class ModePrivate;

class QSGS_CORE_EXPORT Mode
{
protected:
    Mode(const QString &name, QSanguosha::ModeCategory category);

public:
    static QSet<QString> availableModes();
    static const Mode *findMode(const QString &name);

    virtual ~Mode();

    const QString &name() const;
    QSanguosha::ModeCategory category() const;

    virtual int playersCount() const = 0;
    virtual QString roles() const = 0;
    virtual Rule *rule() const = 0;
    virtual IdSet availableCards() const;
    virtual QSet<const General *> availableGenerals() const;

    virtual void startGame(RoomObject *room) const = 0;

private:
    ModePrivate *const d;
    Q_DISABLE_COPY_MOVE(Mode)
};

// I don't think that implementing these modes here is a good idea.
// But there seems to be no other place for them

class GenericRoleModePrivate;

class QSGS_CORE_EXPORT GenericRoleMode : public Mode
{
    explicit GenericRoleMode(const QString &name);

public:
    ~GenericRoleMode() override;

    int playersCount() const override;
    QString roles() const override;
    Rule *rule() const override;

    void startGame(RoomObject *room) const override;

private:
    friend class Mode;
    GenericRoleModePrivate *const d;
};

class GenericHegemonyModePrivate;

class QSGS_CORE_EXPORT GenericHegemonyMode : public Mode
{
    explicit GenericHegemonyMode(const QString &name);

public:
    ~GenericHegemonyMode() override;

    int playersCount() const override;
    QString roles() const override;
    Rule *rule() const override;

    void startGame(RoomObject *room) const override;

private:
    friend class Mode;
    GenericHegemonyModePrivate *const d;
};

#endif
