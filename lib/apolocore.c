/* Copyright (C) 2017, 2019 Luiz Romário Santana Rios
   Copyright (C) 2019 Connor McPherson

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

#define MAX_PIPE_LENGTH 32

/* Lua includes */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* C includes */
#include <stdlib.h>

#include <stdio.h>
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

static int apolocore_job_status(lua_State *L)
{
    check_argc(2);
    check_arg_type(1, LUA_TNUMBER);
    check_arg_type(2, LUA_TBOOLEAN);

    struct native_job_result res = native_job_status(lua_tonumber(L, 1), lua_toboolean(L, 2));

    switch (res.tag) {
        case NATIVE_ERR_NOTFOUND: 
            lua_pushnil(L);
            lua_pushstring(L, "Process not found");
            return 2;
        case NATIVE_ERR_BACKGROUND_UNCHANGED:
            lua_pushnil(L);
            return 1;
        case NATIVE_ERR_BACKGROUND_FINISHED:
            lua_pushstring(L, "finished");
            lua_pushnumber(L, res.exit_code);
            return 2;
        case NATIVE_ERR_BACKGROUND_SUSPENDED:
            lua_pushstring(L, "suspended");
            return 1;
        case NATIVE_ERR_BACKGROUND_RESUMED:
            lua_pushstring(L, "running");
            return 1;
        case NATIVE_ERR_BACKGROUND_FAILED:
            lua_pushstring(L, "failed");
            // Failed processes don't have exit codes
            return 1;
        default:
            lua_pushnil(L);
            lua_pushstring(L, "Unknown error (most likely a bug in apolo)");
            return 2;
    }
}

static int apolocore_job_kill(lua_State *L)
{
    check_argc(2);
    check_arg_type(1, LUA_TNUMBER);
    check_arg_type(2, LUA_TBOOLEAN);
    
    struct native_job_result res = native_job_kill(lua_tonumber(L, 1), lua_toboolean(L, 2));

    switch (res.tag) {
    case NATIVE_ERR_NOTFOUND: 
        lua_pushnil(L);
        lua_pushstring(L, "Process not found");
        return 2;
    case NATIVE_ERR_PERMISSION: 
        lua_pushnil(L);
        lua_pushstring(L, "Process doesn't have the permission to kill that process. Make sure process was started by this program");
        return 2;
    case NATIVE_ERR_SUCCESS:
        lua_pushboolean(L, 1);
        return 1;
    default:
        lua_pushnil(L);
        lua_pushstring(L, "Unknown error (most likely a bug in apolo)");

        return 2;
    }
}

static int apolocore_job_active(lua_State *L)
{
    check_argc(2);
    check_arg_type(1, LUA_TNUMBER);
    check_arg_type(2, LUA_TBOOLEAN);
    
    struct native_job_result res = native_job_set_active(lua_tonumber(L, 1), lua_toboolean(L, 2));

    switch (res.tag) {
        case NATIVE_ERR_NOTFOUND: 
            lua_pushnil(L);
            lua_pushstring(L, "Process not found");
            return 2;
        case NATIVE_ERR_PERMISSION:
            lua_pushnil(L);
            lua_pushstring(L, "Process doesn't have the permission to change that process. Make sure process was started by this program");
            return 2;
        case NATIVE_ERR_SUCCESS:
            lua_pushboolean(L, 1);
            return 1;
        default:
            lua_pushnil(L);
            lua_pushstring(L, "Unknown error (most likely a bug in apolo)");

            return 2;
    }
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
    //Negative indices won't count the nil being added, so decrease to compensate
    if (index < 0)
        index--;

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

/* apolo.core.run(exe_commands, envstrings, is_background, is_eval, pipe_length) */
static int apolocore_execute(lua_State *L)
{
    check_argc(12);
    check_arg_type(1, LUA_TTABLE);
    check_arg_type(2, LUA_TTABLE);
    check_arg_type(3, LUA_TBOOLEAN);
    check_arg_type(4, LUA_TBOOLEAN);
    check_arg_type(5, LUA_TNUMBER);
    check_arg_type(6, LUA_TSTRING);
    check_arg_type(7, LUA_TSTRING);
    check_arg_type(8, LUA_TBOOLEAN);
    check_arg_type(9, LUA_TSTRING);
    check_arg_type(10, LUA_TBOOLEAN);
    check_arg_type(11, LUA_TBOOLEAN);
    check_arg_type(12, LUA_TBOOLEAN);

    {
        int len = lua_tonumber(L, 5);
        const char *executable[MAX_PIPE_LENGTH];
        const char *exeargs[MAX_PIPE_LENGTH][32];
        const char *envstrings[512];  /* Make room for parent environment */

        /* Build executable and exeargs for each table in the table at parameter 1 */
        lua_pushvalue(L, 1);
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            // copy the key so that lua_tostring does not modify the original
            lua_pushvalue(L, -2);
            const int key = (int) lua_tonumber(L, -1) - 1;
            // Populate exeargs with inner table
            table_to_strarray(L, -2, exeargs[key]);

            // Read first value of inner table for executable
            lua_pushnil(L);
            lua_next(L, -3);

            executable[key] = lua_tostring(L, -1);

            // pop value + copy of key, leaving original key
            lua_pop(L, 4);
        }
        // Pop table
        lua_pop(L, 1);

        /* Store env vars in an array */
        table_to_strarray(L, 2, envstrings);

        /* Set up opts */
        enum exec_opts_t opts = EXEC_OPTS_INVALID;
        if (lua_toboolean(L, 3))
            opts = opts | EXEC_OPTS_BG;
        if (lua_toboolean(L, 4))
            opts = opts | EXEC_OPTS_EVAL;
        if (lua_toboolean(L, 8))
            opts = opts | EXEC_OPTS_APPEND_TO;
        if (lua_toboolean(L, 10))
            opts = opts | EXEC_OPTS_APPEND_ERR;
        if (lua_toboolean(L, 11))
            opts = opts | EXEC_OPTS_ERR_TO_OUT;
        if (lua_toboolean(L, 12))
            opts = opts | EXEC_OPTS_OUT_TO_ERR;

        const char* source = lua_tostring(L, 6);
        const char* target = lua_tostring(L, 7);
        const char* err_target = lua_tostring(L, 9);

        //If the IO-redirection files are empty strings, that means there should be no redirection
        if (source[0] == 0)
            source = NULL;
        if (target[0] == 0)
            target = NULL;
        if (err_target[0] == 0)
            err_target = NULL;

        struct native_run_result proc = native_setup_proc_out(opts, target, err_target);
        if (proc.tag == NATIVE_ERR_IN_EXECUTE) {
            for (int pipe=len-1; pipe >= 0; pipe--) {
                if (proc.tag != NATIVE_ERR_IN_EXECUTE) {
                    break;
                }
                proc = native_execute(executable[pipe], exeargs[pipe], envstrings,
                    opts, proc, pipe, source);
            }
            if (proc.tag == NATIVE_ERR_IN_EXECUTE) {
                proc = native_execute_begin(proc, opts);
            }
        }

        switch (proc.tag) {
        case NATIVE_ERR_FORKFAILED:
            lua_pushnil(L);
            lua_pushstring(L, "Failed to fork");

            return 2;
        case NATIVE_ERR_NOTFOUND: 
            lua_pushnil(L);
            lua_pushstring(L, "Command not found");

            return 2;
        case NATIVE_ERR_FILE_NOTFOUND:
            lua_pushnil(L);
            lua_pushstring(L, "File not found");

            return 2;
        case NATIVE_ERR_PIPE_FAILED:
            lua_pushnil(L);
            lua_pushstring(L, "Pipe creation failed");

            return 2;
        case NATIVE_ERR_INTERRUPT:
            lua_pushnil(L);
            lua_pushstring(L, "Command interrupted before completion");

            return 2;
        case NATIVE_ERR_PERMISSION:
            lua_pushnil(L);
            lua_pushstring(L, "Process doesn't have proper permissions to open file");

            return 2;
        case NATIVE_ERR_MAX:
            lua_pushnil(L);
            lua_pushstring(L, "Process has too many files/file descriptors open. Increase the maximum and try again");

            return 2;
        case NATIVE_ERR_VARIABLE_SIZE:
            lua_pushnil(L);
            lua_pushstring(L, "File name exceeded max length");

            return 2;
        case NATIVE_ERR_BACKGROUND_SUCCESS:
            lua_pushnumber(L, proc.pid);
            
            return 1;
        case NATIVE_ERR_SUCCESS:
            if (opts & EXEC_OPTS_EVAL) {
                lua_pushstring(L, proc.out_string);

                return 1;
            }
            else {
                lua_pushboolean(L, proc.exit_code == 0);
                lua_pushnumber(L, proc.exit_code);

                return 2;
            }
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
    {"job_status", apolocore_job_status},
    {"job_kill", apolocore_job_kill},
    {"job_active", apolocore_job_active},
    {"listdirentries", apolocore_listdirentries},
    {"mkdir", apolocore_mkdir},
    {"move", apolocore_move},
    {"rmdir", apolocore_rmdir},
    {"execute", apolocore_execute},
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
