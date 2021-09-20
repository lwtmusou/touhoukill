#include "general.h"
#include "client.h"
#include "engine.h"
#include "package.h"
#include "settings.h"
#include "skill.h"

using namespace QSanguosha;

class GeneralPrivate
{
public:
    const Package *package;
    QString name;
    QString kingdom;
    int maxHp;
    bool lord;
    QSanguosha::Gender gender;
    QSet<QString> skills;
    QStringList relatedSkills;
    bool hidden;
    bool neverShown;
    QStringList companions;
    int headMaxHpAdjustedValue;
    int deputyMaxHpAdjustedValue;

    GeneralPrivate(Package *package, const QString &name, const QString &kingdom, int maxHp, bool isLord, bool male, bool hidden, bool neverShown)
        : package(package)
        , name(name)
        , kingdom(kingdom)
        , maxHp(maxHp)
        , lord(isLord)
        , gender(male ? Male : Female)
        , hidden(hidden)
        , neverShown(neverShown)
        , headMaxHpAdjustedValue(0)
        , deputyMaxHpAdjustedValue(0)
    {
    }
};

General::General(Package *package, const QString &name, const QString &kingdom, int maxHp, bool isLord, bool male, bool hidden, bool neverShown)
    : d(new GeneralPrivate(package, name, kingdom, maxHp, isLord, male, hidden, neverShown))
{
}

QString General::name() const
{
    return d->name;
}

int General::getMaxHp() const
{
    return d->maxHp;
}

QString General::getKingdom() const
{
    return d->kingdom;
}

bool General::isMale() const
{
    return d->gender == Male;
}

bool General::isFemale() const
{
    return d->gender == Female;
}

bool General::isNeuter() const
{
    return d->gender == Neuter;
}

void General::setGender(Gender gender)
{
    d->gender = gender;
}

Gender General::getGender() const
{
    return d->gender;
}

bool General::isLord() const
{
    return d->lord;
}

bool General::isHidden() const
{
    return d->hidden;
}

bool General::isTotallyHidden() const
{
    return d->neverShown;
}

void General::addSkill(const QString &skill_name)
{
    d->skills << skill_name;
}

bool General::hasSkill(const QString &skill_name) const
{
    return d->skills.contains(skill_name);
}

QList<const Skill *> General::getSkillList(bool relate_to_place, bool head_only) const
{
    QList<const Skill *> skills;
    foreach (const QString &skill_name, d->skills) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        Q_ASSERT(skill != nullptr);
        if (relate_to_place && !skill->relateToPlace(!head_only))
            skills << skill;
        else if (!relate_to_place)
            skills << skill;
    }
    return skills;
}

QList<const Skill *> General::getVisibleSkillList(bool relate_to_place, bool head_only) const
{
    QList<const Skill *> skills;
    foreach (const Skill *skill, getSkillList(relate_to_place, head_only)) {
        if (skill->isVisible())
            skills << skill;
    }

    return skills;
}

QSet<const Skill *> General::getVisibleSkills(bool relate_to_place, bool head_only) const
{
    QList<const Skill *> list = getVisibleSkillList(relate_to_place, head_only);
    return QSet<const Skill *>(list.begin(), list.end());
}

QSet<const TriggerSkill *> General::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skills;
    foreach (QString skill_name, d->skills) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill != nullptr)
            skills << skill;
    }
    return skills;
}

void General::addRelateSkill(const QString &skill_name)
{
    d->relatedSkills << skill_name;
}

QStringList General::getRelatedSkillNames() const
{
    return d->relatedSkills;
}

QString General::getPackage() const
{
    return d->package->name();
}

QString General::getSkillDescription(bool include_name, bool yellow) const
{
    QString description;

    foreach (const Skill *skill, getVisibleSkillList()) {
        QString skill_name = Sanguosha->translate(skill->objectName());
        QString desc = skill->getDescription();
        desc.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
        description.append(
            QStringLiteral("<font color=%1><b>%2</b>:</font> %3 <br/> <br/>").arg(yellow ? QStringLiteral("#FFFF33") : QStringLiteral("#FF0080")).arg(skill_name).arg(desc));
    }

    if (include_name) {
        QString color_str = Sanguosha->getKingdomColor(d->kingdom).name();
        QString g_name = Sanguosha->translate(QStringLiteral("!") + name());
        if (g_name.startsWith(QStringLiteral("!")))
            g_name = Sanguosha->translate(name());
        QString name = QStringLiteral("<font color=%1><b>%2</b></font>     ").arg(color_str).arg(g_name);
        name.prepend(QStringLiteral("<img src='image/kingdom/icon/%1.png'/>    ").arg(d->kingdom));
        for (int i = 0; i < d->maxHp; i++)
            name.append(QStringLiteral("<img src='image/system/magatamas/5.png' height = 12/>"));
        if (hasSkill(QStringLiteral("banling"))) {
            for (int i = 0; i < d->maxHp; i++)
                name.append(QStringLiteral("<img src='image/system/magatamas/1.png' height = 12/>"));
        }
        name.append(QStringLiteral("<br/> <br/>"));
        description.prepend(name);
    }

    return description;
}

void General::addCompanion(const QString &name)
{
    d->companions << name;
}

bool General::isCompanionWith(const QString &name) const
{
    const General *other = Sanguosha->getGeneral(name);
    Q_ASSERT(other);
    return d->companions.contains(name) || other->d->companions.contains(d->name);
}

QString General::getCompanions() const
{
    QStringList name;
    foreach (const QString &general, d->companions)
        name << QStringLiteral("%1").arg(Sanguosha->translate(general));
    QStringList generals = Sanguosha->getGenerals();
    foreach (QString gname, generals) {
        const General *gnr = Sanguosha->getGeneral(gname);
        if (gnr == nullptr)
            continue;
        if (gnr->d->companions.contains(d->name))
            name << QStringLiteral("%1").arg(Sanguosha->translate(gnr->name()));
    }
    return name.join(QStringLiteral(" "));
}

void General::setHeadMaxHpAdjustedValue(int adjusted_value /* = -1 */)
{
    d->headMaxHpAdjustedValue = adjusted_value;
}

void General::setDeputyMaxHpAdjustedValue(int adjusted_value /* = -1 */)
{
    d->deputyMaxHpAdjustedValue = adjusted_value;
}

int General::getMaxHpHead() const
{
    return d->maxHp + d->headMaxHpAdjustedValue;
}

int General::getMaxHpDeputy() const
{
    return d->maxHp + d->deputyMaxHpAdjustedValue;
}
