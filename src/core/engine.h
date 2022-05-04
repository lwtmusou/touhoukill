#ifndef TOUHOUKILL_ENGINE_H
#define TOUHOUKILL_ENGINE_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG

#include "global.h"
#include "json.h"
#include "protocol.h"
#include "qsgscore.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

class QVersionNumber;
class LuaStatePointer;
class CardPattern;
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

class EnginePrivate;
#endif

class QSGS_CORE_EXPORT Engine final
{
public:
    ~Engine();

    static Engine *instance();

    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate) const;

    QString versionDate() const;
    QString getVersion() const;
    const QVersionNumber &versionNumber() const;
    QString modName() const;

    QStringList kingdoms() const;
    QStringList hegemonyKingdoms() const;

    const CardPattern *responsePattern(const QString &name) const;
    const CardPattern *expPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;

    void addPackage(const Package *package);
    QList<const Package *> packages() const;
    const Package *findPackage(const QString &name) const;
    QStringList packageNames() const;

    const General *general(const QString &name) const;
    QStringList generalNames() const;
    QSet<QString> availableLords() const;
    QSet<QString> latestGenerals() const;

    const Skill *skill(const QString &skill_name) const;
    const Skill *skill(const EquipCard *card) const;
    QStringList skillNames() const;
    void addSkills(const QList<const Skill *> &skills);

    int cardCount() const;
    const CardDescriptor &cardDescriptor(int cardId) const;
    void registerCardFace(const CardFace *face);
    const CardFace *cardFace(const QString &name) const;
    void unregisterCardFace(const QString &name);

    void registerTrigger(const Trigger *trigger);
    const Trigger *trigger(const QString &name) const;
    void unregisterTrigger(const QString &name);

    QVariant config(const QString &key) const;

#ifndef SWIG
    // Don't expose functions which are to be removed to SWIG

    // move to Mode?
    int availableGeneralCount() const;
    QSet<QString> getRandomLords() const;
    QSet<QString> getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;
    QList<int> getRandomCards() const;

#ifndef QSGS_CORE_NODEPRECATED

public:
#else

private:
#endif
    Q_DECL_DEPRECATED QString getModeName(const QString &name) const;
    Q_DECL_DEPRECATED int getPlayerCount(const QString &name) const;
    Q_DECL_DEPRECATED QString getRoles(const QString &mode) const;
    Q_DECL_DEPRECATED QStringList getRoleList(const QString &mode) const;
    Q_DECL_DEPRECATED bool isGeneralHidden(const QString &general_name) const;
#endif

private:
    Engine();
    Q_DISABLE_COPY_MOVE(Engine)
    EnginePrivate *const d;
};

#ifdef SWIG
extern Engine *const Sanguosha;
#else
#define Sanguosha (Engine::instance())
#endif

#endif
