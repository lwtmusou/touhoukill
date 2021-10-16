#ifndef _GENERAL_H
#define _GENERAL_H

#include "global.h"

#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>

class Skill;
class Package;

class GeneralPrivate;

class General final
{
public:
    explicit General(Package *package, const QString &name, const QString &kingdom, int maxHp = 4, bool isLord = false, bool male = false, bool hidden = false,
                     bool neverShown = false);

    QString name() const;

    // property getters/setters
    int getMaxHp() const;
    QString getKingdom() const;
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;
    bool isLord() const;
    bool isHidden() const;
    bool isTotallyHidden() const;

    int getMaxHpHead() const;
    int getMaxHpDeputy() const;

    QSanguosha::Gender getGender() const;
    void setGender(QSanguosha::Gender gender);

    void addSkill(const QString &skill_name);
    bool hasSkill(const QString &skill_name) const;
    QList<const Skill *> getSkillList(bool relate_to_place = false, bool head_only = true) const;
    QList<const Skill *> getVisibleSkillList(bool relate_to_place = false, bool head_only = true) const;
    QSet<const Skill *> getVisibleSkills(bool relate_to_place = false, bool head_only = true) const;

    // only for general overview?
    void addRelateSkill(const QString &skill_name);
    QStringList getRelatedSkillNames() const;

    QString getPackage() const;
    void addCompanion(const QString &name);
    bool isCompanionWith(const QString &name) const;
    QString getCompanions() const;

    void setHeadMaxHpAdjustedValue(int adjusted_value = -1);
    void setDeputyMaxHpAdjustedValue(int adjusted_value = -1);

private:
    Q_DISABLE_COPY_MOVE(General)
    GeneralPrivate *const d;
};

#endif
