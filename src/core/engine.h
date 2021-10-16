#ifndef _ENGINE_H
#define _ENGINE_H

#include "global.h"
#include "json.h"
#include "protocol.h"

// TODO: kill this

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMutex>
#include <QStringList>
#include <QThread>

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

class Engine final
{
public:
    Engine();
    ~Engine();

    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate) const;

    void addPackage(const Package *package);
    void addBanPackage(const QString &package_name);
    QStringList getBanPackages() const;
    QString getVersionNumber() const;
    QString getVersion() const;
    QString getVersionName() const;
    QVersionNumber getQVersionNumber() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QStringList getHegemonyKingdoms() const;
    QColor getKingdomColor(const QString &kingdom) const;
    QStringList getChattingEasyTexts() const;
    QString getSetupString() const;

    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const QString &mode) const;
    int getPlayerCount(const QString &mode) const;
    QString getRoles(const QString &mode) const;
    QStringList getRoleList(const QString &mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;
    QSanguosha::HandlingMethod getCardHandlingMethod(const QString &method_name) const;

    QList<const Package *> getPackages() const;
    const Package *findPackage(const QString &name) const;

    const General *getGeneral(const QString &name) const;
    QStringList getGenerals() const;
    int getGeneralCount(bool include_banned = false) const;
    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const CardDescriptor &getEngineCard(int cardId) const;

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QStringList getLatestGenerals(const QSet<QString> &ban_set = QSet<QString>()) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;

    bool isGeneralHidden(const QString &general_name) const;

    QStringList LordBGMConvertList;
    QStringList LordBackdropConvertList;
    QStringList LatestGeneralList;
    int operationTimeRate(QSanProtocol::CommandType command, const QVariant &msg) const;

    QVariant getConfigFromConfigFile(const QString &key) const;

    void registerCardFace(const CardFace *face);
    const CardFace *cardFace(const QString &name);
    void unregisterCardFace(const QString &name);

private:
    Q_DISABLE_COPY_MOVE(Engine)
    EnginePrivate *const d;
};

extern Engine *Sanguosha;

#endif
