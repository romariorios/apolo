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

#define check_args(L, argc) \
{\
    int n = lua_gettop((L));\
    if (n != (argc))\
        return luaL_error((L), "Wrong number of arguments (expected %d, got %d)", (argc), n);\
}

static int apolocore_chdir(lua_State *L)
{
    check_args(L, 1);

    lua_pushboolean(L, native_chdir(lua_tostring(L, 1)));
    return 1;
}

static int apolocore_curdir(lua_State *L)
{
    char dir[512];
    check_args(L, 0);

    native_curdir(dir);
    lua_pushstring(L, dir);
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
    check_args(L, 3);

    {
        const char *executable = lua_tostring(L, 1);
        const char *exeargs[32];
        const char *envstrings[128];  /* Make room for parent environment */
        int success;

        /* Store executable args in an array */
        exeargs[0] = executable;
        table_to_strarray(L, 2, &exeargs[1]);

        /* Store env vars in an array */
        table_to_strarray(L, 3, envstrings);

        success = native_run(executable, exeargs, envstrings);

        lua_pushboolean(L, success);

        return 1;
    }
}

static const struct luaL_Reg apolocore[] = {
    {"chdir", apolocore_chdir},
    {"curdir", apolocore_curdir},
    {"run", apolocore_run},
    {NULL, NULL}
};

int luaopen_apolocore(lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, apolocore, 0);
    return 1;
}
