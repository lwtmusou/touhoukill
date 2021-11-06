
%{
namespace CardFaceLuaCall {

// assuming { CardFace.targetFixed } is on top of stack (i.e. at -1)
bool targetFixed(lua_State *l, const Player *player, const Card *card)
{
    SWIG_NewPointerObj(l, player, SWIGTYPE_p_Player, 0); // { player, CardFace.targetFixed }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, player, CardFace.targetFixed }

    int call = lua_pcall(l, 2, 1, 0); // { cardFace.targetFixed() / error }
    return call == LUA_OK;
}

// assuming { CardFace.targetsFeasible } is on top of stack (i.e. at -1)
bool targetsFeasible(lua_State *l, const QList<const Player *> &targets, const Player *Self, const Card *card)
{
    SWIG_NewPointerObj(l, &targets, SWIGTYPE_p_QListT_Player_const_p_t, 0); // { targets, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, Self, SWIGTYPE_p_Player, 0); // { Self, targets, CardFace.targetsFeasible }
    SWIG_NewPointerObj(l, card, SWIGTYPE_p_Card, 0); // { card, Self, targets, CardFace.targetsFeasible }

    int call = lua_pcall(l, 3, 1, 0); // { cardFace.targetsFeasible() / error }
    return call == LUA_OK;
}
}

%}
