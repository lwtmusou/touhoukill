%naturalvar LuaFunction;
%typemap(in) LuaFunction
%{
if (lua_isfunction(L, $input)) {
    lua_pushvalue(L, $input);
    $1 = luaL_ref(L, LUA_REGISTRYINDEX);
} else {
    $1 = 0;
}
%}

%typemap(out) LuaFunction
%{
lua_rawgeti(L, LUA_REGISTRYINDEX, $1);
SWIG_arg ++;
%}

// binding of QString to lua basic type string
// -------------------------------------------
// Since there are no embedding zeros in QString, we strip down embedding zeros when converting to QString

%naturalvar QString;

%typemap(in, checkfn = "SWIG_lua_isnilstring") QString
%{
    if (!lua_isnil(L, $input))
        $1 = QString::fromUtf8(lua_tostring(L, $input));
%}

%typemap(in, checkfn = "SWIG_lua_isnilstring") const QString & ($*1_ltype temp)
%{
    if (!lua_isnil(L, $input))
        temp = QString::fromUtf8(lua_tostring(L, $input));
    $1 = &temp;
%}

%typemap(typecheck, precedence = SWIG_TYPECHECK_STRING) QString {
    $1 = SWIG_lua_isnilstring(L, $input);
}

%typemap(typecheck, precedence = SWIG_TYPECHECK_STRING) const QString & {
    $1 = SWIG_lua_isnilstring(L, $input);
}

%typemap(out) QString
%{ lua_pushstring(L, $1.toUtf8().constData()); SWIG_arg++; %}

%naturalvar QByteArray;

%typemap(out) QByteArray
%{ lua_pushlstring(L, $1.constData(), $1.length()); SWIG_arg++; %}

// -------------------------------------------

%naturalvar QStringList;

%typemap(in, checkfn = "lua_istable") QStringList
%{
for (int i = 0; i < lua_rawlen(L, $input); i++) {
    lua_rawgeti(L, $input, i + 1);
    const char *elem = luaL_checkstring(L, -1);
    $1 << elem;
    lua_pop(L, 1);
}
%}

%typemap(out) QStringList
%{
lua_createtable(L, $1.length(), 0);

for (int i = 0; i < $1.length(); i++) {
    QString str = $1.at(i);
    lua_pushstring(L, str.toUtf8());
    lua_rawseti(L, -2, i + 1);
}

SWIG_arg++;
%}
