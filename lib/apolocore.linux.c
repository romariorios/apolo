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

#include "apolocore.h"

#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern char **environ;

int native_chdir(const char *dir)
{
    // chdir failed if return is -1
    return chdir(dir) != -1;
}

void native_curdir(char *dir)
{
    getcwd(dir, 512);
}

int native_fillentryarray(lua_State *L, const char *dirname)
{
    DIR *dir;
    struct dirent *entry;
    int i;
    const char *type;

    errno = 0;

    if ((dir = opendir(dirname)) == NULL)
        return 0;

    for (i = 1; (entry = readdir(dir)) != NULL; ++i) {
        switch (entry->d_type) {
        // stantdard, multi-platform types
        case DT_DIR:
            type = "dir";
            break;
        case DT_REG:
            type = "file";
            break;

        // exterded types for linux
        case DT_BLK:
            type = "blkdev";
            break;
        case DT_CHR:
            type = "chrdev";
            break;
        case DT_FIFO:
            type = "namedpipe";
            break;
        case DT_LNK:
            type = "symlink";
            break;
        case DT_SOCK:
            type = "udsocket";
            break;
        default:
            type = "unknown";
            break;
        }

        insert_direntry(L, i, entry->d_name, type);
    }

    return 1;
}

int native_run(
    const char *executable, const char **exeargs, const char **envstrings)
{
    const char **env = envstrings;
    char **parent_env = environ;

    for (; *env; ++env);  /* go to end of array */

    /* Copy parent env to child env array */
    for (; *parent_env; ++parent_env, ++env)
        *env = *parent_env;
    *env = NULL;

    /* Fork the process to avoid the script being replaced by execvp */
    pid_t res = fork();
    if (res < 0)  /* Failed to fork, return false */
        return 0;

    if (res != 0) {  /* We're the parent, return */
        int exit_code;
        waitpid(res, &exit_code, 0);
        return 1;
    }

    return execvpe(executable, exeargs, envstrings); /* never returns */
}
