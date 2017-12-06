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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern char **environ;

const char *apolocore_os = "linux";

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

    closedir(dir);
    return 1;
}

int native_mkdir(const char *dir)
{
    struct stat st = {0};

    // If directory already exists, return false
    if (stat(dir, &st) != -1)
        return 0;

    // rwx permissions to owner
    // rx to everyone else
    mkdir(dir, 0755);
    return 1;
}

int native_rmdir(const char *dir)
{
    // noop, since Linux uses Lua's os.remove instead
    return 0;
}

struct native_run_result native_run(
    const char *executable, const char **exeargs, const char **envstrings)
{
    const char **env = envstrings;
    char **parent_env = environ;

    for (; *env; ++env);  /* go to end of array */

    /* Copy parent env to child env array */
    for (; *parent_env; ++parent_env, ++env)
        *env = *parent_env;
    *env = NULL;

    /* Create pipe to communicate execvpe failure in the child */
    int pipe_fd[2];
    int execvpe_errno = 0;

    pipe(pipe_fd);

    /* Fork the process to avoid the script being replaced by execvp */
    pid_t fork_res = fork();
    if (fork_res < 0) {
        struct native_run_result res = {NATIVE_RUN_FORKFAILED, 0};
        return res;
    }

    if (fork_res != 0) {  /* We're the parent, return */
        close(pipe_fd[1]);
        read(pipe_fd[0], &execvpe_errno, sizeof(execvpe_errno));

        struct native_run_result res;
        switch (execvpe_errno) {
        case 0:  // success
            break;
        case ENOENT:
            res.tag = NATIVE_RUN_NOTFOUND;
            return res;

        // TODO treat other exec errors

        default:
            return res;
        }

        int exit_code;
        waitpid(fork_res, &exit_code, 0);

        res.tag = NATIVE_RUN_SUCCESS;
        res.exit_code = WEXITSTATUS(exit_code);
        return res;
    }

    execvpe(executable, exeargs, envstrings);  /* should never return */

    /* If execvpe ever returns, an error occurred: */
    close(pipe_fd[0]);

    execvpe_errno = errno;
    write(pipe_fd[1], &execvpe_errno, sizeof(execvpe_errno));

    exit(0);
}
