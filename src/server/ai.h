#ifndef _AI_H
#define _AI_H

class Room;
class TrickCard;
class ResponseSkill;

struct lua_State;

typedef int LuaFunction;

#include "card.h"
#include "roomthread.h"
#include "serverplayer.h"

#include <QObject>
#include <QString>

class AI : public QObject
{
    Q_OBJECT
    Q_ENUMS(Relation)

public:
    explicit AI(ServerPlayer *player);
    ~AI() override;

    enum Relation
    {
        Friend,
        Enemy,
        Neutrality
    };
    static Relation GetRelation3v3(const ServerPlayer *a, const ServerPlayer *b);
    static Relation GetRelation(const ServerPlayer *a, const ServerPlayer *b);
    Relation relationTo(const ServerPlayer *other) const;
    bool isFriend(const ServerPlayer *other) const;
    bool isEnemy(const ServerPlayer *other) const;

    QList<ServerPlayer *> getEnemies() const;
    QList<ServerPlayer *> getFriends() const;

    virtual void activate(CardUseStruct &card_use) = 0;
    virtual Card::Suit askForSuit(const QString &reason) = 0;
    virtual QString askForKingdom() = 0;
    virtual bool askForSkillInvoke(const QString &skill_name, const QVariant &data) = 0;
    virtual QString askForChoice(const QString &skill_name, const QString &choices, const QVariant &data) = 0;
    virtual QList<int> askForDiscard(const QString &reason, int discard_num, int min_num, bool optional, bool include_equip) = 0;
    virtual const Card *askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive) = 0;
    virtual int askForCardChosen(ServerPlayer *who, const QString &flags, const QString &reason, Card::HandlingMethod method) = 0;
    virtual const Card *askForCard(const QString &pattern, const QString &prompt, const QVariant &data) = 0;
    virtual QString askForUseCard(const QString &pattern, const QString &prompt, const Card::HandlingMethod method) = 0;
    virtual int askForAG(const QList<int> &card_ids, bool refusable, const QString &reason) = 0;
    virtual const Card *askForCardShow(ServerPlayer *requestor, const QString &reason) = 0;
    virtual const Card *askForPindian(ServerPlayer *requestor, const QString &reason) = 0;
    virtual ServerPlayer *askForPlayerChosen(const QList<ServerPlayer *> &targets, const QString &reason, bool optional) = 0;
    virtual const Card *askForSinglePeach(ServerPlayer *dying) = 0;
    virtual ServerPlayer *askForYiji(const QList<int> &cards, const QString &reason, int &card_id) = 0;
    virtual void askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type) = 0;
    virtual void filterEvent(TriggerEvent triggerEvent, const QVariant &data);

protected:
    Room *room;
    ServerPlayer *self;
};

class TrustAI : public AI
{
    Q_OBJECT

public:
    explicit TrustAI(ServerPlayer *player);
    ~TrustAI() override;

    void activate(CardUseStruct &card_use) override;
    Card::Suit askForSuit(const QString &) override;
    QString askForKingdom() override;
    bool askForSkillInvoke(const QString &skill_name, const QVariant &data) override;
    QString askForChoice(const QString &skill_name, const QString &choices, const QVariant &data) override;
    QList<int> askForDiscard(const QString &reason, int discard_num, int min_num, bool optional, bool include_equip) override;
    const Card *askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive) override;
    int askForCardChosen(ServerPlayer *who, const QString &flags, const QString &reason, Card::HandlingMethod method) override;
    const Card *askForCard(const QString &pattern, const QString &prompt, const QVariant &data) override;
    QString askForUseCard(const QString &pattern, const QString &prompt, const Card::HandlingMethod method) override;
    int askForAG(const QList<int> &card_ids, bool refusable, const QString &reason) override;
    const Card *askForCardShow(ServerPlayer *requestor, const QString &reason) override;
    const Card *askForPindian(ServerPlayer *requestor, const QString &reason) override;
    ServerPlayer *askForPlayerChosen(const QList<ServerPlayer *> &targets, const QString &reason, bool optional) override;
    const Card *askForSinglePeach(ServerPlayer *dying) override;
    ServerPlayer *askForYiji(const QList<int> &cards, const QString &reason, int &card_id) override;
    void askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type) override;

    virtual bool useCard(const Card *card);

private:
    ResponseSkill *response_skill;
};

class LuaAI : public TrustAI
{
    Q_OBJECT

public:
    explicit LuaAI(ServerPlayer *player);

    const Card *askForCardShow(ServerPlayer *requestor, const QString &reason) override;
    bool askForSkillInvoke(const QString &skill_name, const QVariant &data) override;
    void activate(CardUseStruct &card_use) override;
    QString askForUseCard(const QString &pattern, const QString &prompt, const Card::HandlingMethod method) override;
    QList<int> askForDiscard(const QString &reason, int discard_num, int min_num, bool optional, bool include_equip) override;
    const Card *askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive) override;
    QString askForChoice(const QString &skill_name, const QString &choices, const QVariant &data) override;
    int askForCardChosen(ServerPlayer *who, const QString &flags, const QString &reason, Card::HandlingMethod method) override;
    const Card *askForCard(const QString &pattern, const QString &prompt, const QVariant &data) override;
    ServerPlayer *askForPlayerChosen(const QList<ServerPlayer *> &targets, const QString &reason, bool optional) override;
    int askForAG(const QList<int> &card_ids, bool refusable, const QString &reason) override;
    const Card *askForSinglePeach(ServerPlayer *dying) override;
    const Card *askForPindian(ServerPlayer *requestor, const QString &reason) override;
    Card::Suit askForSuit(const QString &reason) override;

    ServerPlayer *askForYiji(const QList<int> &cards, const QString &reason, int &card_id) override;
    void askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type) override;

    void filterEvent(TriggerEvent triggerEvent, const QVariant &data) override;

    LuaFunction callback;

private:
    void pushCallback(lua_State *L, const char *function_name);
    void pushQIntList(lua_State *L, const QList<int> &list);
    void reportError(lua_State *L);
    bool getTable(lua_State *L, QList<int> &table);
};

#endif
