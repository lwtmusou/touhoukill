#ifndef qsgslegacy__STRUCTS_H
#define qsgslegacy__STRUCTS_H

#include "global.h"
#include "structs.h"

#include <QList>

class Player;

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

struct SlashEffectStruct
{
    SlashEffectStruct();

    int jink_num;

    const Card *slash;
    const Card *jink;

    Player *from;
    Player *to;

    int drank;

    QSanguosha::DamageNature nature;
    bool multiple;
    bool nullified;
    bool canceled;
    QList<int> effectValue;
};

struct JinkEffectStruct
{
    JinkEffectStruct();

    SlashEffectStruct slashEffect;
    const Card *jink;
};

struct ChoiceMadeStruct
{
    inline ChoiceMadeStruct()
        : player(nullptr)
        , type(NoChoice)
    {
    }

    enum ChoiceType
    {
        NoChoice,

        SkillInvoke,
        SkillChoice,
        Nullification,
        CardChosen,
        CardResponded,
        CardUsed,
        AGChosen,
        CardShow,
        Peach,
        TriggerOrder,
        ReverseFor3v3,
        Activate,
        Suit,
        Kingdom,
        CardDiscard,
        CardExchange,
        ViewCards,
        PlayerChosen,
        Rende,
        Yiji,
        Pindian,

        NumOfChoices
    };

    Player *player;
    ChoiceType type;
    QStringList args;
    QVariant m_extraData;
};

namespace ExtendCardUseStruct {
bool isValid(const CardUseStruct &use, const QString &pattern);
bool tryParse(CardUseStruct &use, const QVariant &usage, RoomObject *room);
QString toString(const CardUseStruct &use);
} // namespace ExtendCardUseStruct

namespace ExtendCardMoveReason {
bool tryParse(CardMoveReason &reason, const QVariant &);
QVariant toVariant(const CardMoveReason &);
} // namespace ExtendCardMoveReason

namespace ExtendTriggerDetail {
QVariant toVariant(const TriggerDetail &detail);
QStringList toList(const TriggerDetail &detail);
} // namespace ExtendTriggerDetail

Q_DECLARE_METATYPE(LegacyCardsMoveStruct)
Q_DECLARE_METATYPE(LegacyCardsMoveOneTimeStruct)
Q_DECLARE_METATYPE(SlashEffectStruct)
Q_DECLARE_METATYPE(JinkEffectStruct)
Q_DECLARE_METATYPE(ChoiceMadeStruct)

#endif
