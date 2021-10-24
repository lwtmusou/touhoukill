#include "CardFace.h"
#include "lua-wrapper.h"
#include "lua.hpp"

namespace SgsEx {
// Make sure to keep this same with sgs_ex.lua
enum TableType
{
    FirstTypeMask = 0xf00,
    SecondTypeMask = 0x0f0,
    ThirdTypeMask = 0x00f,

    Package = 0x000,
    CardPackage = 0x001,
    GeneralPackage = 0x002,

    CardFace = 0x100,

    BasicCard = 0x110,

    TrickCard = 0x120,
    NonDelayedTrick = 0x121,
    DelayedTrick = 0x122,

    EquipCard = 0x130,
    Weapon = 0x131,
    Armor = 0x132,
    DefensiveHorse = 0x133,
    OffensiveHorse = 0x134,
    Treasure = 0x135,

    SkillCard = 0x140,

    Skill = 0x200,
    ViewAsSkill = 0x210,
    FilterSkill = 0x220,
    ProhibitSkill = 0x230,
    DistanceSkill = 0x240,
    MaxCardsSkill = 0x250,
    AttackRangeSkill = 0x260,
    TreatAsEquippingSkill = 0x270,

    Trigger = 0x300,

    CardDescriptor = 0x400,

    GeneralDescriptor = 0x500,
};

class LuaCardPrivate
{
public:
    QString name;
};

class LuaBasicCard : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaBasicCard(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaBasicCard() override
    {
        delete d;
    }

    QString subTypeName() const override
    {
        // todo: implementation
        return QString();
    }

private:
    LuaCardPrivate *const d;
};

class LuaWeapon : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaWeapon(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaWeapon() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaArmor(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaArmor() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaDefensiveHorse : public DefensiveHorse
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaDefensiveHorse(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaDefensiveHorse() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaOffensiveHorse : public OffensiveHorse
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaOffensiveHorse(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaOffensiveHorse() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaTreasure : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaTreasure(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaTreasure() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaNonDelayedTrick : public NonDelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaNonDelayedTrick(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaNonDelayedTrick() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

class LuaDelayedTrick : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaDelayedTrick(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaDelayedTrick() override
    {
        delete d;
    }

    void takeEffect(Player *target) const override
    {
        // todo: implementation
    }

private:
    LuaCardPrivate *const d;
};

class LuaSkillCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit LuaSkillCard(const QString &name)
        : d(new LuaCardPrivate)
    {
        d->name = name;
    }
    ~LuaSkillCard() override
    {
        delete d;
    }

private:
    LuaCardPrivate *const d;
};

namespace {
#define TYPEPAIR(t) std::make_pair((t), &Lua##t ::staticMetaObject)
static const QHash<TableType, const QMetaObject *> typeTable({
    TYPEPAIR(BasicCard),
    TYPEPAIR(Weapon),
    TYPEPAIR(Armor),
    TYPEPAIR(DefensiveHorse),
    TYPEPAIR(OffensiveHorse),
    TYPEPAIR(Treasure),
    TYPEPAIR(NonDelayedTrick),
    TYPEPAIR(DelayedTrick),
    TYPEPAIR(SkillCard),
});
#undef TYPEPAIR

::CardFace *newCardFaceByType(TableType t, const QString &name)
{
    const QMetaObject *o = typeTable.value(t, nullptr);
    if (o == nullptr)
        return nullptr;

    QObject *r = o->newInstance(Q_ARG(const QString &, name));
    if (r == nullptr)
        return nullptr;

    ::CardFace *f = qobject_cast<::CardFace *>(r);
    if (f == nullptr)
        delete r;

    return f;
}
} // namespace

// [-0, +0, e]
// assuming sgs_ex.CardFaces is at top of stack
::CardFace *createNewLuaCardFace(const QString &name)
{
    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    ::CardFace *r = nullptr;
    QString errorMessage;

    // it should contain an item calls name
    // [sgs_ex.CardFaces[name], sgs_ex.CardFaces]
    // may call metamethods thus may raise error. Need a method to deal with error since longjmp breaks call stack
    // or we can use rawget
    int type = lua_getfield(l, -1, name.toUtf8().constData());
    do {
        // sgs_ex.CardFaces[name] should be a table
        if (type != LUA_TTABLE) {
            if (type == LUA_TNIL)
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"] does not exist")).arg(name);
            else
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"] is not table")).arg(name);
            break;
        }

        type = lua_getfield(l, -1, "type");
        do {
            if (type != LUA_TNUMBER) {
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"].type is not number")).arg(name);
                break;
            }

            // create cardface by its type
            int ok = 0;
            TableType t = static_cast<TableType>(lua_tonumberx(l, 1, &ok));
            if (!ok) {
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"].type is not number")).arg(name);
                break;
            }

            r = newCardFaceByType(t, name);
            if (r == nullptr) {
                errorMessage = QString(QStringLiteral("No corressponding CardFace which sgs_ex.CardFaces[\"%1\"].type is %2")).arg(name, QString::number(static_cast<int>(t)));
                break;
            }
        } while (0);
        lua_pop(l, 1);
    } while (0);
    lua_pop(l, 1);

    if (r == nullptr) {
        lua_pushstring(l, errorMessage.toUtf8().constData());
        lua_error(l);
    }

    return r;
}

} // namespace SgsEx

#include "luacardface.moc"
