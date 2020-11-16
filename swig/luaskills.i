
class EquipSkill : public TriggerSkill
{
public:
    static bool equipAvailable(const ServerPlayer *p, EquipCard::Location location, const QString &equip_name);
    static bool equipAvailable(const ServerPlayer *p, const EquipCard *card);

#if SWIGVERSION >= 0x030000
private:
    EquipSkill() = delete;
#endif
};

%{

#include "lua-wrapper.h"
#include <QMessageBox>

static void Error(lua_State *L)
{
    const char *error_string = lua_tostring(L, -1);
    lua_pop(L, 1);
    QMessageBox::warning(NULL, "Lua script error!", error_string);
}

%}
