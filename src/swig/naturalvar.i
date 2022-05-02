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
