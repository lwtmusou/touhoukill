#ifndef TOUHOUKILL_GENERAL_H
#define TOUHOUKILL_GENERAL_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"

#include <QSet>
#include <QString>
#include <QStringList>

class Skill;
class Package;

class GeneralPrivate;
#endif

class QSGS_CORE_EXPORT General final
{
public:
#ifndef SWIG
    explicit General(Package *package, const QString &name, const QString &kingdom, int maxHp = 4, bool isLord = false, QSanguosha::Gender gender = QSanguosha::Female,
                     bool hidden = false, bool neverShown = false);
#endif

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
    QSet<QString> companions() const;

    void setHeadMaxHpAdjustedValue(int adjusted_value = -1);
    void setDeputyMaxHpAdjustedValue(int adjusted_value = -1);

private:
    General() = delete;
    Q_DISABLE_COPY_MOVE(General)
    GeneralPrivate *const d;
};

#endif
