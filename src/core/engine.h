#ifndef TOUHOUKILL_ENGINE_H
#define TOUHOUKILL_ENGINE_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#endif

class QVersionNumber;
class General;
class ViewAsSkill;
class ProhibitSkill;
class TreatAsEquippingSkill;
class EquipCard;
class TargetModSkill;
class Package;
class DistanceSkill;
class Skill;
class MaxCardsSkill;
class AttackRangeSkill;
class Player;
struct CardDescriptor;
class Card;
class RoomObject;
class CardFace;
class Trigger;
class Mode;

#ifndef SWIG
class LuaStatePointer;
class CardPattern;
class EnginePrivate;
#endif

class QSGS_CORE_EXPORT Engine final
{
public:
#ifndef SWIG
    explicit Engine();
    ~Engine();

    void init();
#endif

    // translations
    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate) const;

    // version related
    QString versionDate() const;
    QString version() const;
    const QVersionNumber &versionNumber() const;
    QString modName() const;

    // kingdoms (from configurations)
    QSet<QString> kingdoms() const;
    QSet<QString> hegemonyKingdoms() const;

    // patterns (to be refactored since the 2 patterns confuses people)
    const CardPattern *responsePattern(const QString &name) const;
    const CardPattern *expPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;

    // packages
    void addPackage(const Package *package);
    QList<const Package *> packages() const;
    const Package *findPackage(const QString &name) const;
    QSet<QString> packageNames() const;

    // generals
    const General *general(const QString &name) const;
    QSet<const General *> generals() const;
    QSet<QString> generalNames() const;
    QSet<QString> availableLordNames() const;
    QSet<QString> latestGeneralNames() const;

    // skills
    const Skill *skill(const QString &skill_name) const;
    const Skill *skill(const EquipCard *card) const;
    QStringList skillNames() const;
    void addSkill(const Skill *skill);

    // cardDescriptors
    int cardCount() const;
    const CardDescriptor &cardDescriptor(int cardId) const;
    void registerCardFace(const CardFace *face);
    const CardFace *cardFace(const QString &name) const;
    void unregisterCardFace(const QString &name);

    // triggers
    void registerTrigger(const Trigger *trigger);
    const Trigger *trigger(const QString &name) const;
    void unregisterTrigger(const QString &name);

    QSet<QString> availableGameModes() const;
    const Mode *gameMode(const QString &name) const;

    // configuration
    QVariant configuration(const QString &key) const;

#ifndef QSGS_CORE_NODEPRECATED

public:
#else

private:
#endif
    Q_DECL_DEPRECATED int getPlayerCount(const QString &name) const;
    Q_DECL_DEPRECATED QString getRoles(const QString &mode) const;
    Q_DECL_DEPRECATED QStringList getRoleList(const QString &mode) const;
    Q_DECL_DEPRECATED bool isGeneralHidden(const QString &general_name) const;

private:
    Q_DISABLE_COPY_MOVE(Engine)
    EnginePrivate *const d;
};

#ifdef SWIG
extern Engine *const Sanguosha;
#else
QSGS_CORE_EXPORT Engine *EngineInstanceFunc();
#define Sanguosha (EngineInstanceFunc())
#endif

#endif
