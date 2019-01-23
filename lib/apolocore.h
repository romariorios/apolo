/* Copyright (C) 2017, 2019 Luiz Romário Santana Rios

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

#ifndef APOLOCORE_H
#define APOLOCORE_H

#include <lua.h>

enum native_err {
    NATIVE_ERR_INVALID = 0x0,

    NATIVE_ERR_FORKFAILED,
    NATIVE_ERR_INUSE,
    NATIVE_ERR_NOTFOUND,
    NATIVE_ERR_PERMISSION,

    NATIVE_ERR_BACKGROUND_SUCCESS,
    NATIVE_ERR_SUCCESS
};

void insert_direntry(lua_State *L, int index, const char *dirname, const char *type);

int native_chdir(const char *dir);
int native_copy(const char *orig, const char *dest);
void native_curdir(char *dir);
int native_exists(const char *path);
int native_fillentryarray(lua_State *L, const char *dir);
int native_mkdir(const char *dir);
int native_move(const char *orig, const char *dest);
int native_rmdir(const char *dir);

struct native_run_result
{
    enum native_err tag;
    int exit_code;
};

struct native_run_result native_run(
    const char *executable, const char **exeargs, const char **envstrings,
    int background);

#endif
