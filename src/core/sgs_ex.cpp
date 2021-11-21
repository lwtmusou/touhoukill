#include "CardFace.h"
#include "lua-wrapper.h"
#include "trigger.h"

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
    Rule = 0x310,
    SkillTrigger = 0x320,
    EquipSkillTrigger = 0x321,
    GlobalRecord = 0x330,
    FakeMoveRecord = 0x331,

    CardDescriptor = 0x400,

    GeneralDescriptor = 0x500,
};

namespace {
::CardFace *newCardFaceByType(TableType t, const QString &name)
{
#define NEWCARDFACEBYTYPE(T) \
    if (t == T)              \
        return new ::T(name);

    NEWCARDFACEBYTYPE(BasicCard)
    NEWCARDFACEBYTYPE(NonDelayedTrick)
    NEWCARDFACEBYTYPE(DelayedTrick)
    NEWCARDFACEBYTYPE(Weapon)
    NEWCARDFACEBYTYPE(Armor)
    NEWCARDFACEBYTYPE(DefensiveHorse)
    NEWCARDFACEBYTYPE(OffensiveHorse)
    NEWCARDFACEBYTYPE(Treasure)
    NEWCARDFACEBYTYPE(SkillCard)

#undef NEWCARDFACEBYTYPE

    return nullptr;
}

::Trigger *newTriggerByType(TableType t, const QString &name)
{
    // TODO
    return nullptr;
}
} // namespace

// [-0, +0, e]
::CardFace *createNewLuaCardFace(const QString &name)
{
    LuaStatePointer l = LuaMultiThreadEnvironment::luaStateForCurrentThread();
    ::CardFace *r = nullptr;
    QString errorMessage;

    bool pushed = l->pushCardFace(name); // { CardFaces[name](if successful) }
    if (!pushed) {
        errorMessage = QString(QStringLiteral("Could not find register for card face %1")).arg(name);
    } else {
        // it should contain an item calls name
        // may call metamethods thus may raise error. Need a method to deal with error since longjmp breaks call stack
        // or we can use rawget

        int type = lua_getfield(l, -1, "type"); // { CardFaces[name].type, CardFaces[name] }
        do {
            if (type != LUA_TNUMBER) {
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"].type is not number")).arg(name);
                break;
            }

            // create cardface by its type
            int ok = 0;
            TableType t = static_cast<TableType>(lua_tonumberx(l, 1, &ok));
            if (!(bool)(ok)) {
                errorMessage = QString(QStringLiteral("sgs_ex.CardFaces[\"%1\"].type is not number")).arg(name);
                break;
            }

            r = newCardFaceByType(t, name);
            if (r == nullptr) {
                errorMessage = QString(QStringLiteral("No corressponding CardFace which sgs_ex.CardFaces[\"%1\"].type is %2")).arg(name, QString::number(static_cast<int>(t)));
                break;
            }
        } while (false);
        lua_pop(l, 1); // { CardFaces[name] }
        lua_pop(l, 1); // { }
    }

    if (r == nullptr) {
        lua_pushstring(l, errorMessage.toUtf8().constData());
        lua_error(l);
    }

    return r;
}

} // namespace SgsEx
