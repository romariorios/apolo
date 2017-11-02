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

#include "apolocoregui.h"

#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

int dialog_show(lua_State *L)
{
    int n = lua_gettop(L);
    const char *type;
    const char *title;
    const char *body;
    enum dialog_type d_type;

    if (n != 3)
        return luaL_error(L, "Wrong number of arguments (expected 3, got %d)", n);

    // Some boilerplate to prepare the args from Lua to call the native dialog
    type = lua_tostring(L, 1);
    title = lua_tostring(L, 2);
    body = lua_tostring(L, 3);

    if (strcmp(type, "info") == 0)
        d_type = Info;
    else if (strcmp(type, "warning") == 0)
        d_type = Warning;
    else if (strcmp(type, "question") == 0)
        d_type = Question;
    else if (strcmp(type, "fatal") == 0)
        d_type = Fatal;

    lua_pushboolean(L, native_dialog_show(d_type, title, body));

    // Only return if the dialog is a question
    // Otherwise, return nothing
    return d_type == Question? 1 : 0;
}

static const struct luaL_Reg apolocoregui[] = {
    {"show", dialog_show},
    {NULL, NULL}
};

int luaopen_apolocoregui(lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, apolocoregui, 0);
    return 1;
}
