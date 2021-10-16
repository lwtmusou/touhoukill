#ifndef QSANGUOSHA_MODE_H
#define QSANGUOSHA_MODE_H

#include "global.h"

#include <QSet>
#include <QString>

class Rule;
class Package;
class RoomObject;
class General;

class ModePrivate;

class Mode
{
public:
    Mode(const QString &name, QSanguosha::ModeCategory category);
    virtual ~Mode();

    const QString &name() const;
    QSanguosha::ModeCategory category() const;

    virtual int playersCount() const = 0;
    virtual Rule *rule() const = 0;
    virtual IDSet availableCards() const = 0;
    virtual QSet<const General *> availableGenerals() const = 0;

    virtual void startGame(RoomObject *room) const = 0;

private:
    ModePrivate *const d;
    Q_DISABLE_COPY_MOVE(Mode)
};

// I don't think that implementing these modes here is a good idea.
// But there seems to be no other place for them

class GenericRoleModePrivate;

class GenericRoleMode : public Mode
{
    GenericRoleMode(int loyalistCount, int rebelCount, int renegadeCount);
    ~GenericRoleMode() override;

public:
    int playersCount() const override;
    Rule *rule() const override;
    IDSet availableCards() const override;
    QSet<const General *> availableGenerals() const override;

    void startGame(RoomObject *room) const override;

private:
    GenericRoleModePrivate *const d;
};

class GenericHegemonyModePrivate;

class GenericHegemonyMode : public Mode
{
    GenericHegemonyMode(int players);
    ~GenericHegemonyMode() override;

public:
    int playersCount() const override;
    Rule *rule() const override;
    IDSet availableCards() const override;
    QSet<const General *> availableGenerals() const override;

    void startGame(RoomObject *room) const override;

private:
    GenericHegemonyModePrivate *const d;
};

#endif
