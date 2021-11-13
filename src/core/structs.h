#ifndef _STRUCTS_H
#define _STRUCTS_H

#include "global.h"

#ifndef SWIG
class Skill;
class RoomObject;
class Card;
class Player;

#include <QVariant>
#else
#define Q_DECLARE_METATYPE(...)
#endif

struct DamageStruct
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

struct RecoverStruct
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

struct CardUseStruct
{
    explicit CardUseStruct(const Card *card = nullptr, Player *from = nullptr, const QList<Player *> &to = QList<Player *>(), bool isOwnerUse = true);
    CardUseStruct(const Card *card, Player *from, Player *target, bool isOwnerUse = true);
    CardUseStruct(const Card *card, Player *from, const Card *toCard, bool isOwnerUse = true);

#ifndef SWIG
    bool isValid(const QString &pattern) const;
    void parse(const QString &str, RoomObject *room);
    bool tryParse(const QVariant &usage, RoomObject *room);
    QString toString() const;
#endif

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

struct CardEffectStruct
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

class CardMoveReason
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
    bool tryParse(const QVariant &);
    QVariant toVariant() const;

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

struct SingleCardMoveStruct
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

// Can't specialize QListSpecialMethods<SingleCardMoveStruct> because it modifies size of QList and may cause binary incompatibility
struct CardsMoveStruct : public QList<SingleCardMoveStruct>
{
    bool isLastHandCard;
    CardMoveReason reason;
};

#ifndef SWIG
struct LegacyCardsMoveStruct
{
    LegacyCardsMoveStruct();
    LegacyCardsMoveStruct(const QList<int> &ids, Player *from, Player *to, QSanguosha::Place from_place, QSanguosha::Place to_place, const CardMoveReason &reason);
    LegacyCardsMoveStruct(const QList<int> &ids, Player *to, QSanguosha::Place to_place, const CardMoveReason &reason);
    LegacyCardsMoveStruct(int id, Player *from, Player *to, QSanguosha::Place from_place, QSanguosha::Place to_place, const CardMoveReason &reason);
    LegacyCardsMoveStruct(int id, Player *to, QSanguosha::Place to_place, const CardMoveReason &reason);
    bool operator==(const LegacyCardsMoveStruct &other) const;
    bool operator<(const LegacyCardsMoveStruct &other) const;

    QList<int> card_ids; // TODO: Replace with IDSet
    QSanguosha::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    CardMoveReason reason;
    bool open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;

    QSanguosha::Place origin_from_place, origin_to_place;
    Player *origin_from, *origin_to;
    QString origin_from_pile_name, origin_to_pile_name; //for case of the movement transitted
    QList<int> broken_ids; //record broken equip IDs from EquipPlace
    QList<int> shown_ids; //record broken shown IDs from HandPlace

    bool tryParse(const QVariant &arg);
    QVariant toVariant() const;
    bool isRelevant(const Player *player) const;
};

struct LegacyCardsMoveOneTimeStruct
{
    QList<int> card_ids;
    QList<QSanguosha::Place> from_places;
    QSanguosha::Place to_place;
    CardMoveReason reason;
    Player *from, *to;
    QStringList from_pile_names;
    QString to_pile_name;

    QList<QSanguosha::Place> origin_from_places;
    QSanguosha::Place origin_to_place;
    Player *origin_from, *origin_to;
    QStringList origin_from_pile_names;
    QString origin_to_pile_name; //for case of the movement transitted

    QList<bool> open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;
    QList<int> broken_ids; //record broken equip IDs from EquipPlace
    QList<int> shown_ids; //record broken shown IDs from HandPlace

    inline void removeCardIds(const QList<int> &to_remove)
    {
        foreach (int id, to_remove) {
            int index = card_ids.indexOf(id);
            if (index != -1) {
                card_ids.removeAt(index);
                from_places.removeAt(index);
                from_pile_names.removeAt(index);
                open.removeAt(index);
            }
        }
    }
};
#endif

struct DeathStruct
{
    explicit DeathStruct(Player *who = nullptr, DamageStruct *damage = nullptr);

    Player *who; // who is dead
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
    Player *nowAskingForPeaches; // who is asking for peaches

    Player *viewAsKiller;
    bool useViewAsKiller;
};

using DyingStruct = DeathStruct;

struct PindianStruct
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

struct JudgeStruct
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

struct PhaseChangeStruct
{
    explicit PhaseChangeStruct(Player *player = nullptr, QSanguosha::Phase from = QSanguosha::PhaseNotActive, QSanguosha::Phase to = QSanguosha::PhaseNotActive);

    Player *player;
    QSanguosha::Phase from;
    QSanguosha::Phase to;
};

struct PhaseSkippingStruct
{
    PhaseSkippingStruct(Player *player = nullptr, QSanguosha::Phase phase = QSanguosha::PhaseNotActive, bool isCost = false);

    Player *player;
    QSanguosha::Phase phase;
    bool isCost;
};

struct CardResponseStruct
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

struct MarkChangeStruct
{
    explicit MarkChangeStruct(Player *player = nullptr, const QString &name = QString(), int num = 1);

    Player *player;
    QString name;
    int num;
};

struct SkillAcquireDetachStruct
{
    explicit SkillAcquireDetachStruct(Player *player = nullptr, const Skill *skill = nullptr, bool isAcquire = false);

    Player *player;
    const Skill *skill;
    bool isAcquire;
};

struct CardAskedStruct
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

class TriggerDetail
{
public:
    explicit TriggerDetail(RoomObject *room, const Trigger *trigger = nullptr, const QString &name = QString(), Player *owner = nullptr, Player *invoker = nullptr,
                           const QList<Player *> &targets = QList<Player *>(), bool isCompulsory = false, bool effectOnly = false);
    TriggerDetail(RoomObject *room, const Trigger *trigger, const QString &name, Player *owner, Player *invoker, Player *target, bool isCompulsory = false,
                  bool effectOnly = false);

    TriggerDetail(const TriggerDetail &other);
    TriggerDetail &operator=(const TriggerDetail &other);
    ~TriggerDetail();

    RoomObject *room() const;
    const Trigger *trigger() const;
    const QString &name() const;
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

#ifndef SWIG
    bool operator<(const TriggerDetail &arg2) const; // the operator < for sorting the invoke order.
#endif
    // the operator ==. it only judge the skill name, the skill invoker, and the skill owner. it don't judge the skill target because it is chosen by the skill invoker
    bool sameTrigger(const TriggerDetail &arg2) const;
    // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    bool sameTimingWith(const TriggerDetail &arg2) const;
    bool isValid() const; // validity check

    QVariant toVariant() const;
    QStringList toList() const;

private:
#ifndef SWIG
    TriggerDetailPrivate *d;
#endif
};

struct HpLostStruct
{
    explicit HpLostStruct(Player *player = nullptr, int num = 1);

    Player *player;
    int num;
};

struct DrawNCardsStruct
{
    explicit DrawNCardsStruct(Player *player = nullptr, int n = 2, bool isInitial = false);

    Player *player;
    int n;
    bool isInitial;
};

struct SkillInvalidStruct
{
    explicit SkillInvalidStruct(Player *player = nullptr, const Skill *skill = nullptr, bool invalid = false);

    Player *player;
    const Skill *skill;
    bool invalid;
};

struct BrokenEquipChangedStruct
{
    explicit BrokenEquipChangedStruct(Player *player = nullptr, QList<int> ids = {}, bool broken = false, bool moveFromEquip = false);

    Player *player;
    QList<int> ids;
    bool broken;
    bool moveFromEquip;
};

struct ShownCardChangedStruct
{
    explicit ShownCardChangedStruct(Player *player = nullptr, QList<int> ids = {}, bool shown = false, bool moveFromHand = false);

    Player *player;
    QList<int> ids;
    bool shown;
    bool moveFromHand;
};

struct ShowGeneralStruct
{
    explicit ShowGeneralStruct(Player *player = nullptr, int pos = 0, bool isShow = true);

    Player *player;
    int pos;
    bool isShow;
};

struct ExtraTurnStruct
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

Q_DECLARE_METATYPE(LegacyCardsMoveStruct)
Q_DECLARE_METATYPE(LegacyCardsMoveOneTimeStruct)

#endif
