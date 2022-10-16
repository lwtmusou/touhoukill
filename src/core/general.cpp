#include "general.h"
#include "engine.h"
#include "package.h"
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
    Gender gender;
    QSet<QString> skills;
    QStringList relatedSkills;
    bool hidden;
    bool neverShown;
    QSet<QString> companions;
    int headMaxHpAdjustedValue;
    int deputyMaxHpAdjustedValue;

    GeneralPrivate(Package *package, const QString &name, const QString &kingdom, int maxHp, bool isLord, Gender gender, bool hidden, bool neverShown)
        : package(package)
        , name(name)
        , kingdom(kingdom)
        , maxHp(maxHp)
        , lord(isLord)
        , gender(gender)
        , hidden(hidden)
        , neverShown(neverShown)
        , headMaxHpAdjustedValue(0)
        , deputyMaxHpAdjustedValue(0)
    {
    }
};

General::General(Package *package, const QString &name, const QString &kingdom, int maxHp, bool isLord, Gender gender, bool hidden, bool neverShown)
    : d(new GeneralPrivate(package, name, kingdom, maxHp, isLord, gender, hidden, neverShown))
{
    (*package) << this;
}

QString General::name() const
{
    return d->name;
}

int General::maxHp() const
{
    return d->maxHp;
}

QString General::kingdom() const
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
    return d->gender == NonBinaryGender;
}

void General::setGender(Gender gender)
{
    d->gender = gender;
}

Gender General::gender() const
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

QSet<const Skill *> General::skills(bool relate_to_place, bool head_only) const
{
    QSet<const Skill *> skills;
    foreach (const QString &skill_name, d->skills) {
        const Skill *skill = Sanguosha->skill(skill_name);
        Q_ASSERT(skill != nullptr);
        if (relate_to_place && !skill->relateToPlace(!head_only))
            skills << skill;
        else if (!relate_to_place)
            skills << skill;
    }
    return skills;
}

void General::addRelateSkill(const QString &skill_name)
{
    d->relatedSkills << skill_name;
}

QStringList General::relatedSkillNames() const
{
    return d->relatedSkills;
}

const Package *General::package() const
{
    return d->package;
}

QString General::packageName() const
{
    return d->package->name();
}

void General::addCompanion(const QString &name)
{
    d->companions << name;
}

bool General::isCompanionWith(const QString &name) const
{
    const General *other = Sanguosha->general(name);
    Q_ASSERT(other);
    return d->companions.contains(name) || other->d->companions.contains(d->name);
}

QSet<QString> General::companions() const
{
    QSet<QString> ret = d->companions;

    QSet<QString> generals = Sanguosha->generalNames();
    foreach (const QString &gname, generals) {
        const General *gnr = Sanguosha->general(gname);
        if (gnr == nullptr)
            continue;
        if (gnr->d->companions.contains(d->name))
            ret << gname;
    }
    return ret;
}

void General::setHeadMaxHpAdjustedValue(int adjusted_value /* = -1 */)
{
    d->headMaxHpAdjustedValue = adjusted_value;
}

void General::setDeputyMaxHpAdjustedValue(int adjusted_value /* = -1 */)
{
    d->deputyMaxHpAdjustedValue = adjusted_value;
}

int General::maxHpHead() const
{
    return d->maxHp + d->headMaxHpAdjustedValue;
}

int General::maxHpDeputy() const
{
    return d->maxHp + d->deputyMaxHpAdjustedValue;
}
