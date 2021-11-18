#ifndef _GENERAL_H
#define _GENERAL_H

#include "global.h"
#include "qsgscore.h"

#include <QSet>
#include <QString>
#include <QStringList>

class Skill;
class Package;

class GeneralPrivate;

class QSGS_CORE_EXPORT General final
{
public:
    explicit General(Package *package, const QString &name, const QString &kingdom, int maxHp = 4, bool isLord = false, bool male = false, bool hidden = false,
                     bool neverShown = false);

    QString name() const;

    // property getters/setters
    int maxHp() const;
    QString kingdom() const;
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;
    bool isLord() const;
    bool isHidden() const;
    bool isTotallyHidden() const;

    int maxHpHead() const;
    int maxHpDeputy() const;

    QSanguosha::Gender gender() const;
    void setGender(QSanguosha::Gender gender);

    void addSkill(const QString &skill_name);
    bool hasSkill(const QString &skill_name) const;
    QSet<const Skill *> skills(bool relate_to_place = false, bool head_only = true) const;

    // only for general overview?
    void addRelateSkill(const QString &skill_name);
    QStringList relatedSkillNames() const;

    QString getPackage() const;
    void addCompanion(const QString &name);
    bool isCompanionWith(const QString &name) const;
    QString companions() const;

    void setHeadMaxHpAdjustedValue(int adjusted_value = -1);
    void setDeputyMaxHpAdjustedValue(int adjusted_value = -1);

private:
    Q_DISABLE_COPY_MOVE(General)
    GeneralPrivate *const d;
};

#endif
