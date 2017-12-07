/* Copyright (C) 2017 Luiz Romário Santana Rios

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/* Lua includes */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* C includes */
#include <stdlib.h>

/* Apolo includes */
#include "apolocore.h"

extern const char *apolocore_os;

#define check_argc(argc) \
{\
    int n = lua_gettop(L);\
    if (n != (argc))\
        return luaL_error(L, "Wrong number of arguments (expected %d, got %d)", (argc), n);\
}

#define check_arg_type(ind, type) \
{\
    int t = lua_type(L, (ind));\
    if (t != (type))\
        return luaL_error(L, "Wrong type for argument %d", (ind));\
}

static int apolocore_chdir(lua_State *L)
{
    check_argc(1);
    check_arg_type(1, LUA_TSTRING);

    lua_pushboolean(L, native_chdir(lua_tostring(L, 1)));
    return 1;
}

static int apolocore_copy(lua_State *L)
{
    check_argc(2);
    check_arg_type(1, LUA_TSTRING);
    check_arg_type(2, LUA_TSTRING);
    
    lua_pushboolean(
        L,
        native_copy(
            lua_tostring(L, 1),
            lua_tostring(L, 2)));
    return 1;
}

static int apolocore_curdir(lua_State *L)
{
    char dir[512];
    check_argc(0);

    native_curdir(dir);
    lua_pushstring(L, dir);
    return 1;
}

static int apolocore_exists(lua_State *L)
{
    check_argc(1);
    check_arg_type(1, LUA_TSTRING);

    lua_pushboolean(L, native_exists(lua_tostring(L, 1)));
    return 1;
}

// To be called inside of native_fillentryarray
void insert_direntry(lua_State *L, int index, const char *dirname, const char *type)
{
    lua_pushnumber(L, index);

    lua_createtable(L, 0, 2);

    lua_pushstring(L, "name");
    lua_pushstring(L, dirname);
    lua_settable(L, -3);

    lua_pushstring(L, "type");
    lua_pushstring(L, type);
    lua_settable(L, -3);

    lua_settable(L, -3);
}

static int apolocore_listdirentries(lua_State *L)
{
    check_argc(1);
    check_arg_type(1, LUA_TSTRING);

    // Push entries into a new lua table
    lua_newtable(L);
    native_fillentryarray(L, lua_tostring(L, 1));

    // Return table pushed to the stack at lua_createtable
    return 1;
}

static int apolocore_mkdir(lua_State *L)
{
    check_argc(1);
    check_arg_type(1, LUA_TSTRING);

    // return native_mkdir's return
    lua_pushboolean(L, native_mkdir(lua_tostring(L, 1)));
    return 1;
}

static int apolocore_move(lua_State *L)
{
    check_argc(2);
    check_arg_type(1, LUA_TSTRING);
    check_arg_type(2, LUA_TSTRING);
    
    lua_pushboolean(
        L,
        native_move(
            lua_tostring(L, 1),
            lua_tostring(L, 2)));
    return 1;
}

static int apolocore_rmdir(lua_State *L)
{
    check_argc(1);
    check_arg_type(1, LUA_TSTRING);

    lua_pushboolean(L, native_rmdir(lua_tostring(L, 1)));
    return 1;
}

static void table_to_strarray(lua_State *L, int index, const char **strarray)
{
    int i = 0;

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        if (!lua_isnumber(L, -2))  /* if current index is not a number, ignore */
            continue;

        strarray[i++] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    // Sentinel indicating the end of the array
    strarray[i] = NULL;
}

/* apolo.core.run(executable, exeargs, envstrings) */
static int apolocore_run(lua_State *L)
{
    check_argc(3);
    check_arg_type(1, LUA_TSTRING);
    check_arg_type(2, LUA_TTABLE);
    check_arg_type(3, LUA_TTABLE);

    {
        const char *executable = lua_tostring(L, 1);
        const char *exeargs[32];
        const char *envstrings[128];  /* Make room for parent environment */
        struct native_run_result res;

        /* Store executable args in an array */
        exeargs[0] = executable;
        table_to_strarray(L, 2, &exeargs[1]);

        /* Store env vars in an array */
        table_to_strarray(L, 3, envstrings);

        res = native_run(executable, exeargs, envstrings);

        switch (res.tag) {
        case NATIVE_ERR_FORKFAILED:
            lua_pushnil(L);
            lua_pushstring(L, "Failed to fork");

            return 2;
        case NATIVE_ERR_NOTFOUND:
            lua_pushnil(L);
            lua_pushstring(L, "Command not found");

            return 2;
        case NATIVE_ERR_SUCCESS:
            lua_pushboolean(L, res.exit_code == 0);
            lua_pushnumber(L, res.exit_code);

            return 2;
        default:
            lua_pushnil(L);
            lua_pushstring(L, "Unknown error (most likely a bug in apolo)");

            return 2;
        }
    }
}

static const struct luaL_Reg apolocore[] = {
    {"chdir", apolocore_chdir},
    {"copy", apolocore_copy},
    {"curdir", apolocore_curdir},
    {"exists", apolocore_exists},
    {"listdirentries", apolocore_listdirentries},
    {"mkdir", apolocore_mkdir},
    {"move", apolocore_move},
    {"rmdir", apolocore_rmdir},
    {"run", apolocore_run},
    {NULL, NULL}
};

int luaopen_apolocore(lua_State *L)
{
    lua_newtable(L);

    luaL_setfuncs(L, apolocore, 0);

    lua_pushstring(L, "osname");
    lua_pushstring(L, apolocore_os);
    lua_settable(L, -3);

    return 1;
}
