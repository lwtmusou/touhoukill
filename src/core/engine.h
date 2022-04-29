#ifndef _ENGINE_H
#define _ENGINE_H

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

class EnginePrivate;

class QSGS_CORE_EXPORT Engine final
{
public:
    Engine();
    ~Engine();

    // move to ServerInfo
    void addBanPackage(const QString &package_name);
    QStringList getBanPackages() const;

    // move to UI
    QStringList getChattingEasyTexts() const;
    QStringList LordBGMConvertList;
    QStringList LordBackdropConvertList;

    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate) const;

    QString getVersionDate() const;
    QString getVersion() const;
    const QVersionNumber &getQVersionNumber() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QStringList getHegemonyKingdoms() const;

    // move to Mode?
    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const QString &mode) const;
    int getPlayerCount(const QString &mode) const;
    QString getRoles(const QString &mode) const;
    QStringList getRoleList(const QString &mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;

    void addPackage(const Package *package);
    QList<const Package *> getPackages() const;
    const Package *findPackage(const QString &name) const;

    const General *getGeneral(const QString &name) const;
    QStringList getGenerals() const;
    int getGeneralCount(bool include_banned = false) const;
    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QStringList getLatestGenerals(const QSet<QString> &ban_set = QSet<QString>()) const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;
    QStringList LatestGeneralList;
    bool isGeneralHidden(const QString &general_name) const;

    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const CardDescriptor &getEngineCard(int cardId) const;
    void registerCardFace(const CardFace *face);
    const CardFace *cardFace(const QString &name);
    void unregisterCardFace(const QString &name);
    QList<int> getRandomCards() const;

    QVariant getConfigFromConfigFile(const QString &key) const;

public:
    static const int S_SERVER_TIMEOUT_GRACIOUS_PERIOD;

private:
    Q_DISABLE_COPY_MOVE(Engine)
    EnginePrivate *const d;
};

extern QSGS_CORE_EXPORT Engine *Sanguosha;

#endif
