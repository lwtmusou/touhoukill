#ifndef THKILL_GENERAL_H
#define THKILL_GENERAL_H

class Skill;
class TriggerSkill;
class Package;
class QSize;

#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>

class General : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString kingdom READ getKingdom CONSTANT)
    Q_PROPERTY(int maxhp READ getMaxHp CONSTANT)
    Q_PROPERTY(bool male READ isMale STORED false CONSTANT)
    Q_PROPERTY(bool female READ isFemale STORED false CONSTANT)
    Q_PROPERTY(Gender gender READ getGender CONSTANT)
    Q_PROPERTY(bool lord READ isLord CONSTANT)
    Q_PROPERTY(bool hidden READ isHidden CONSTANT)

public:
    explicit General(Package *package, const QString &name, const QString &kingdom, int max_hp = 4, bool male = false, bool hidden = false, bool never_shown = false);

    // property getters/setters
    [[nodiscard]] int getMaxHp() const;
    [[nodiscard]] QString getKingdom() const;
    [[nodiscard]] bool isMale() const;
    [[nodiscard]] bool isFemale() const;
    [[nodiscard]] bool isNeuter() const;
    [[nodiscard]] bool isLord() const;
    [[nodiscard]] bool isHidden() const;
    [[nodiscard]] bool isTotallyHidden() const;

    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] int getMaxHpHead() const;
    [[nodiscard]] int getMaxHpDeputy() const;

    enum Gender
    {
        Sexless,
        Male,
        Female,
        Neuter
    };
    Q_ENUM(Gender)

    [[nodiscard]] Gender getGender() const;
    void setGender(Gender gender);

    void addSkill(Skill *skill);
    void addSkill(const QString &skill_name);
    [[nodiscard]] bool hasSkill(const QString &skill_name) const;
    [[nodiscard]] QList<const Skill *> getSkillList(bool relate_to_place = false, bool head_only = true) const;
    [[nodiscard]] QList<const Skill *> getVisibleSkillList(bool relate_to_place = false, bool head_only = true) const;
    [[nodiscard]] QSet<const Skill *> getVisibleSkills(bool relate_to_place = false, bool head_only = true) const;
    [[nodiscard]] QSet<const TriggerSkill *> getTriggerSkills() const;

    void addRelateSkill(const QString &skill_name);
    [[nodiscard]] QStringList getRelatedSkillNames() const;

    [[nodiscard]] QString getPackage() const;
    [[nodiscard]] QString getSkillDescription(bool include_name = false, bool yellow = true) const;
    void addCompanion(const QString &name);
    [[nodiscard]] bool isCompanionWith(const QString &name) const;
    [[nodiscard]] QString getCompanions() const;

    void setHeadMaxHpAdjustedValue(int adjusted_value = -1);
    void setDeputyMaxHpAdjustedValue(int adjusted_value = -1);

    [[nodiscard]] inline QSet<QString> getExtraSkillSet() const
    {
        return extra_set;
    }

public slots:
    void lastWord() const;

private:
    QString kingdom;
    int max_hp;
    Gender gender;
    bool lord;
    QSet<QString> skill_set;
    QSet<QString> extra_set;
    QStringList skillname_list;
    QStringList related_skills;
    bool hidden;
    bool never_shown;
    QStringList companions;
    int head_max_hp_adjusted_value;
    int deputy_max_hp_adjusted_value;
};

#endif
