#ifndef TOUHOUKILL_STRUCTS_H
#define TOUHOUKILL_STRUCTS_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"

#include <QString>
#include <QStringList>
#include <QVariant>

class Skill;
class RoomObject;
class Card;
class Player;
#endif

struct QSGS_CORE_EXPORT DamageStruct
{
    explicit DamageStruct(const Card *card = nullptr, Player *from = nullptr, Player *to = nullptr, int damage = 1, QSanguosha::DamageNature nature = QSanguosha::DamageNormal);
    explicit DamageStruct(const QString &reason, Player *from = nullptr, Player *to = nullptr, int damage = 1, QSanguosha::DamageNature nature = QSanguosha::DamageNormal);

    Player *from;
    Player *to;
    const Card *card;
    int damage;
    QSanguosha::DamageNature nature;
    bool chain;
    bool transfer;
    bool by_user;
    QString reason;
    bool trigger_chain;
    QString trigger_info; //keep addtion info while record. since this damage event may be triggered lately by insertion of new damage event.
};

struct QSGS_CORE_EXPORT RecoverStruct
{
    // keep same argument sequence of DamageStruct
    explicit RecoverStruct(const Card *card = nullptr, Player *from = nullptr, Player *to = nullptr, int recover = 1);
    explicit RecoverStruct(const QString &reason, Player *from = nullptr, Player *to = nullptr, int recover = 1);

    int recover;
    Player *from;
    Player *to;
    const Card *card;
    QString reason;
};

struct QSGS_CORE_EXPORT CardUseStruct
{
    explicit CardUseStruct(const Card *card = nullptr, Player *from = nullptr, const QList<Player *> &to = QList<Player *>(), bool isOwnerUse = true);
    CardUseStruct(const Card *card, Player *from, Player *target, bool isOwnerUse = true);
    CardUseStruct(const Card *card, Player *from, const Card *toCard, bool isOwnerUse = true);

    const Card *card;
    Player *from;
    QList<Player *> to;
    const Card *toCard;
    bool m_isOwnerUse;
    bool m_addHistory;
    bool m_isHandcard;
    bool m_isLastHandcard;
    QList<int> m_showncards;
    QStringList nullified_list;
    QSanguosha::CardUseReason m_reason;
};

struct QSGS_CORE_EXPORT CardEffectStruct
{
    explicit CardEffectStruct(const Card *card = nullptr, Player *from = nullptr, Player *to = nullptr);

    const Card *card;
    Player *from;
    Player *to;
    bool multiple; // helper to judge whether the card has multiple targets, does not make sense if the card inherits SkillCard
    bool nullified;
    bool canceled; // for cancel process, like "yuyi"
    QList<int> effectValue;
};

class QSGS_CORE_EXPORT CardMoveReason
{
public:
    QSanguosha::MoveReasonCategory m_reason;
    QString m_playerId; // the cause (not the source) of the movement, such as "lusu" when "dimeng", or "zhanghe" when "qiaobian"
    QString m_targetId; // To keep this structure lightweight, currently this is only used for UI purpose.
    // It will be set to empty if multiple targets are involved. NEVER use it for trigger condition
    // judgement!!! It will not accurately reflect the real reason.
    QString m_skillName; // skill that triggers movement of the cards, such as "longdang", "dimeng"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
    QVariant m_extraData; // additional data and will not be parsed to clients
    QVariant m_provider; // additional data recording who provide this card for otherone to use or response, e.g. guanyu provide a slash for "jijiang"

    /* implicit */ CardMoveReason(QSanguosha::MoveReasonCategory moveReason = QSanguosha::MoveReasonUnknown, const QString &playerId = QString(),
                                  const QString &skillName = QString(), const QString &eventName = QString());
    CardMoveReason(QSanguosha::MoveReasonCategory moveReason, const QString &playerId, const QString &targetId, const QString &skillName, const QString &eventName);

#ifndef SWIG

    inline bool operator==(const CardMoveReason &other) const
    {
        return m_reason == other.m_reason && m_playerId == other.m_playerId && m_targetId == other.m_targetId && m_skillName == other.m_skillName
            && m_eventName == other.m_eventName;
    }
    inline bool operator!=(const CardMoveReason &other) const
    {
        return !((*this) == other);
    }
#endif
};

// Attention!!! DO NOT FORWARD DECLARE SingleCardMoveStruct
// Always using #include <structs.h>
struct QSGS_CORE_EXPORT SingleCardMoveStruct
{
    /* implicit */ SingleCardMoveStruct(int id = -1, Player *to = nullptr, QSanguosha::Place toPlace = QSanguosha::PlaceHand);
    SingleCardMoveStruct(int id, Player *from, Player *to, QSanguosha::Place fromPlace = QSanguosha::PlaceUnknown, QSanguosha::Place toPlace = QSanguosha::PlaceHand);

    // Info about Card:
    int card_id;
    bool broken;
    bool shown;
    bool open;

    // Info about from:
    Player *from;
    QSanguosha::Place fromPlace;
    QString fromPile;

    // Info about to:
    Player *to;
    QSanguosha::Place toPlace;
    QString toPile;

    // Should origin_***  be preserved? Do they make any sense? In what use case should the original ones be picked instead of the modified ones?

public:
#ifndef SWIG
    // satisfy QList
    inline bool operator==(const SingleCardMoveStruct &another) const
    {
        return card_id == another.card_id;
    }
#endif
};

// Can we specialize QListSpecialMethods<SingleCardMoveStruct>?
// It modifies size of QList and may cause binary incompatibility
#ifdef SWIG
struct CardsMoveStruct : public QList<SingleCardMoveStruct>
#else
using CardsMoveStruct = QList<SingleCardMoveStruct>;
template<> struct QSGS_CORE_EXPORT QListSpecialMethods<SingleCardMoveStruct>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    : public QListSpecialMethodsBase<SingleCardMoveStruct>
#endif
#endif
{
    bool isLastHandCard;
    CardMoveReason reason;
};

struct QSGS_CORE_EXPORT DeathStruct
{
    explicit DeathStruct(Player *who = nullptr, DamageStruct *damage = nullptr);

    Player *who; // who is dead
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
    Player *nowAskingForPeaches; // who is asking for peaches

    Player *viewAsKiller;
    bool useViewAsKiller;
};

using DyingStruct = DeathStruct;

struct QSGS_CORE_EXPORT PindianStruct
{
    explicit PindianStruct(Player *from = nullptr, Player *to = nullptr);

    Player *from;
    Player *to;
    const Card *from_card;
    const Card *to_card;
    QSanguosha::Number from_number;
    QSanguosha::Number to_number;
    QString reason;
    bool success;
};

struct QSGS_CORE_EXPORT JudgeStruct
{
    explicit JudgeStruct(Player *who = nullptr, const QString &pattern = QStringLiteral("."), const QString &reason = QString());

    bool isPatternMatch() const;
    inline bool isEffected() const
    {
        return !negative == isPatternMatch();
    }

    Player *who;
    QString pattern;
    QString reason;

    const Card *m_card;

    bool time_consuming;
    bool negative;
    bool play_animation;
    bool ignore_judge; //for tiandao

    // do they really need to be recorded here?
    Player *retrial_by_response; // record whether the current judge card is provided by a response retrial
    Player *relative_player; // record relative player like skill owner of "huazhong", for processing the case like "huazhong -> dizhen -> huazhong"

    // deprecated
#ifndef SWIG
    inline void setCard(const Card *card)
    {
        m_card = card;
    }
    inline const Card *card() const
    {
        return m_card;
    }
#endif
};

struct QSGS_CORE_EXPORT PhaseChangeStruct
{
    explicit PhaseChangeStruct(Player *player = nullptr, QSanguosha::Phase from = QSanguosha::PhaseNotActive, QSanguosha::Phase to = QSanguosha::PhaseNotActive);

    Player *player;
    QSanguosha::Phase from;
    QSanguosha::Phase to;
};

struct QSGS_CORE_EXPORT PhaseSkippingStruct
{
    PhaseSkippingStruct(Player *player = nullptr, QSanguosha::Phase phase = QSanguosha::PhaseNotActive, bool isCost = false);

    Player *player;
    QSanguosha::Phase phase;
    bool isCost;
};

struct QSGS_CORE_EXPORT CardResponseStruct
{
    explicit CardResponseStruct(const Card *card = nullptr, Player *from = nullptr, bool isRetrial = false, bool isProvision = false, Player *to = nullptr);

    const Card *m_card;
    Player *from;
    bool m_isRetrial;
    bool m_isProvision;
    Player *to;
    bool m_isHandcard;
    bool m_isNullified;
    bool m_isShowncard;
};

struct QSGS_CORE_EXPORT MarkChangeStruct
{
    explicit MarkChangeStruct(Player *player = nullptr, const QString &name = QString(), int num = 1);

    Player *player;
    QString name;
    int num;
};

struct QSGS_CORE_EXPORT SkillAcquireDetachStruct
{
    explicit SkillAcquireDetachStruct(Player *player = nullptr, const Skill *skill = nullptr, bool isAcquire = false);

    Player *player;
    const Skill *skill;
    bool isAcquire;
};

struct QSGS_CORE_EXPORT CardAskedStruct
{
    explicit CardAskedStruct(Player *player = nullptr, const QString &pattern = QStringLiteral("."), const QString &prompt = QString(),
                             QSanguosha::HandlingMethod method = QSanguosha::MethodNone);

    Player *player;
    QString pattern;
    QString prompt;
    QSanguosha::HandlingMethod method;
};

#ifndef SWIG
class Trigger;
class TriggerDetailPrivate;
#endif

class QSGS_CORE_EXPORT TriggerDetail
{
public:
    explicit TriggerDetail(RoomObject *room, const Trigger *trigger = nullptr, Player *owner = nullptr, Player *invoker = nullptr,
                           const QList<Player *> &targets = QList<Player *>(), bool isCompulsory = false, bool effectOnly = false);
    TriggerDetail(RoomObject *room, const Trigger *trigger, Player *owner, Player *invoker, Player *target, bool isCompulsory = false, bool effectOnly = false);

    TriggerDetail(const TriggerDetail &other);
    TriggerDetail &operator=(const TriggerDetail &other);
    ~TriggerDetail();

    RoomObject *room() const;
    const Trigger *trigger() const;
    QString name() const;
    Player *owner() const;
    Player *invoker() const;
    QList<Player *> targets() const;
    bool isCompulsory() const;
    bool triggered() const;
    bool effectOnly() const;
    const QVariantMap &tag() const;

    void addTarget(Player *target);
    void setTriggered(bool t);
    QVariantMap &tag();

    bool operator<(const TriggerDetail &arg2) const; // the operator < for sorting the invoke order.
    // the operator ==. it only judge the skill name, the skill invoker, and the skill owner. it don't judge the skill target because it is chosen by the skill invoker
    bool operator==(const TriggerDetail &arg2) const;
    // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    bool sameTimingWith(const TriggerDetail &arg2) const;
    bool isValid() const; // validity check

private:
    TriggerDetailPrivate *d;
};

struct QSGS_CORE_EXPORT HpLostStruct
{
    explicit HpLostStruct(Player *player = nullptr, int num = 1);

    Player *player;
    int num;
};

struct QSGS_CORE_EXPORT DrawNCardsStruct
{
    explicit DrawNCardsStruct(Player *player = nullptr, int n = 2, bool isInitial = false);

    Player *player;
    int n;
    bool isInitial;
};

struct QSGS_CORE_EXPORT SkillInvalidStruct
{
    explicit SkillInvalidStruct(Player *player = nullptr, const Skill *skill = nullptr, bool invalid = false);

    Player *player;
    const Skill *skill;
    bool invalid;
};

struct QSGS_CORE_EXPORT BrokenEquipChangedStruct
{
    explicit BrokenEquipChangedStruct(Player *player = nullptr, QList<int> ids = {}, bool broken = false, bool moveFromEquip = false);

    Player *player;
    QList<int> ids;
    bool broken;
    bool moveFromEquip;
};

struct QSGS_CORE_EXPORT ShownCardChangedStruct
{
    explicit ShownCardChangedStruct(Player *player = nullptr, QList<int> ids = {}, bool shown = false, bool moveFromHand = false);

    Player *player;
    QList<int> ids;
    bool shown;
    bool moveFromHand;
};

struct QSGS_CORE_EXPORT ShowGeneralStruct
{
    explicit ShowGeneralStruct(Player *player = nullptr, int pos = 0, bool isShow = true);

    Player *player;
    int pos;
    bool isShow;
};

struct QSGS_CORE_EXPORT ExtraTurnStruct
{
    explicit ExtraTurnStruct(Player *player = nullptr);

    Player *player;
    QList<QSanguosha::Phase> set_phases;
};

Q_DECLARE_METATYPE(DamageStruct)
Q_DECLARE_METATYPE(CardEffectStruct)
Q_DECLARE_METATYPE(CardUseStruct)
Q_DECLARE_METATYPE(SingleCardMoveStruct)
Q_DECLARE_METATYPE(CardsMoveStruct)
Q_DECLARE_METATYPE(DeathStruct)
Q_DECLARE_METATYPE(RecoverStruct)
Q_DECLARE_METATYPE(PhaseChangeStruct)
Q_DECLARE_METATYPE(CardResponseStruct)
Q_DECLARE_METATYPE(MarkChangeStruct)
Q_DECLARE_METATYPE(SkillAcquireDetachStruct)
Q_DECLARE_METATYPE(CardAskedStruct)
Q_DECLARE_METATYPE(HpLostStruct)
Q_DECLARE_METATYPE(PhaseSkippingStruct)
Q_DECLARE_METATYPE(DrawNCardsStruct)
Q_DECLARE_METATYPE(QList<SkillInvalidStruct>)
Q_DECLARE_METATYPE(JudgeStruct *)
Q_DECLARE_METATYPE(PindianStruct *)
Q_DECLARE_METATYPE(ExtraTurnStruct)
Q_DECLARE_METATYPE(BrokenEquipChangedStruct)
Q_DECLARE_METATYPE(ShownCardChangedStruct)
Q_DECLARE_METATYPE(ShowGeneralStruct)

#endif
