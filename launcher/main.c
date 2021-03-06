/* Copyright (C) 2021 Luiz Romário Santana Rios

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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apolo_lua.h"
#include "../lib/apolocore.h"

#define try_load(__what, __name) \
    { \
        int err = luaL_do##__what(L, __name); \
        if (err != LUA_OK) { \
            const char* error_msg = lua_tostring(L, -1); \
            fprintf(stderr, "Failed to load %s: %s\n", __name, error_msg); \
	    exit(1); \
        } \
    }

static int luaopen_apolo(lua_State *L)
{
    try_load(string, apolo_lua);

    return 1;
}

static void create_arg_table(lua_State *L, int argc, char *argv[])
{
    lua_createtable(L, argc, 0);
    for (int i = 0; i < argc; ++i) {
	lua_pushstring(L, argv[i]);
	lua_seti(L, -2, i - 1);
    }

    lua_setglobal(L, "arg");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
	printf("Usage: %s <apolo script>\n", argv[0]);
	return 1;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "apolocore", luaopen_apolocore, 0);
    luaL_requiref(L, "apolo", luaopen_apolo, 0);

    create_arg_table(L, argc, argv);

    try_load(file, argv[1]);

    return 0;
}
