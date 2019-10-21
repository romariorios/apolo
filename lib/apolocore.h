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

#ifndef APOLOCORE_H
#define APOLOCORE_H

#ifdef APOLO_OS_LINUX
    #include "apolocore.linux.h"
#elif APOLO_OS_WIN
    #include "apolocore.win.h"
#endif

#define EVAL_BUFFER_SIZE 1024

#include <lua.h>

enum native_err {
    NATIVE_ERR_INVALID = 0x0,

    NATIVE_ERR_FORKFAILED,
    NATIVE_ERR_INUSE,
    NATIVE_ERR_NOTFOUND,
    NATIVE_ERR_FILE_NOTFOUND,
    NATIVE_ERR_PERMISSION,
    NATIVE_ERR_PIPE_FAILED,
    NATIVE_ERR_INTERRUPT,
    NATIVE_ERR_MAX,
    NATIVE_ERR_VARIABLE_SIZE,

    NATIVE_ERR_BACKGROUND_SUCCESS,
    NATIVE_ERR_IN_EXECUTE,
    NATIVE_ERR_SUCCESS,
    
    NATIVE_ERR_BACKGROUND_UNCHANGED,
    NATIVE_ERR_BACKGROUND_FINISHED,
    NATIVE_ERR_BACKGROUND_SUSPENDED,
    NATIVE_ERR_BACKGROUND_RESUMED,
    NATIVE_ERR_BACKGROUND_FAILED
};

enum exec_opts_t {
    EXEC_OPTS_INVALID = 0x0,
    EXEC_OPTS_BG = 0x1,
    EXEC_OPTS_EVAL = 0x2,
    EXEC_OPTS_RETURN_ERR = 0x4,
    EXEC_OPTS_APPEND_TO = 0x8,
    EXEC_OPTS_APPEND_ERR = 0x10,
    EXEC_OPTS_ERR_TO_OUT = 0x20,
    EXEC_OPTS_OUT_TO_ERR = 0x40
};


void insert_direntry(lua_State *L, int index, const char *dirname, const char *type);

int native_chdir(const char *dir);
int native_copy(const char *orig, const char *dest);
void native_curdir(char *dir);
int native_exists(const char *path);
struct native_job_result native_job_status(const int pid, int is_wait);
struct native_job_result native_job_kill(const int pid, int is_kill);
struct native_job_result native_job_set_active(const int pid, int is_suspend);
int native_fillentryarray(lua_State *L, const char *dir);
int native_mkdir(const char *dir);
int native_move(const char *orig, const char *dest);
int native_rmdir(const char *dir);

struct native_run_result
{
    enum native_err tag;
    long unsigned exit_code;
    char out_string[EVAL_BUFFER_SIZE];
    int pid;
    
    struct native_pipe_info pipe_info;
};

struct native_job_result
{
    enum native_err tag;
    long unsigned int exit_code;
};

struct native_run_result native_setup_proc_out(enum exec_opts_t opts,
    const char *target_file, const char *err_target_file);

struct native_run_result native_execute(
    const char *executable, const char **exeargs, const char **envstrings,
    enum exec_opts_t opts, struct native_run_result prev_proc, int index, const char *source_file);

struct native_run_result native_execute_begin(struct native_run_result proc,
    enum exec_opts_t opts);

#endif
