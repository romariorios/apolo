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

#include "apolocore.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
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

int native_copy(const char *orig, const char *dest)
{
    // noop, because Linux calls /bin/cp from Lua
    return 0;
}

void native_curdir(char *dir)
{
    getcwd(dir, 512);
}

int native_exists(const char *path)
{
    struct stat st = {0};

    return stat(path, &st) != -1;
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
    // If directory already exists, return false
    if (native_exists(dir))
        return 0;

    // rwx permissions to owner
    // rx to everyone else
    mkdir(dir, 0755);
    return 1;
}

int native_move(const char *orig, const char *dest)
{
    // noop, because Linux calls /bin/mv from Lua
    return 0;
}

int native_rmdir(const char *dir)
{
    // noop, since Linux uses Lua's os.remove instead
    return 0;
}

struct native_run_result native_execute(
    const char *executable, const char **exeargs, const char **envstrings,
    unsigned char background, unsigned char is_eval)
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
    int pipe_output_fd[2];
    int execvpe_errno = 0;

    if (!background)	
        pipe(pipe_fd);
    if(is_eval)
        pipe(pipe_output_fd);

    /* Fork the process to avoid the script being replaced by execvp */
    pid_t fork_res = fork();
    if (fork_res < 0) {
        struct native_run_result res = {NATIVE_ERR_FORKFAILED, 0};
        return res;
    }

    if (fork_res != 0) {  /* We're the parent, return */
        if (!background) {
            close(pipe_fd[1]);
            read(pipe_fd[0], &execvpe_errno, sizeof(execvpe_errno));
        }

        struct native_run_result res = {NATIVE_ERR_INVALID, 0};
        switch (execvpe_errno) {
        case 0:  // success
            break;
        case ENOENT:
            res.tag = NATIVE_ERR_NOTFOUND;
            return res;

        // TODO treat other exec errors

        default:
            return res;
        }

        if (background) {
            res.tag = NATIVE_ERR_BACKGROUND_SUCCESS;
            return res;
        }

        int exit_code;
        waitpid(fork_res, &exit_code, 0);

        //In eval, read the output of the process
        if(is_eval) {
            close(pipe_output_fd[1]);
            //Read bytes
            int num_bytes = read(pipe_output_fd[0] , res.out_string , EVAL_BUFFER_SIZE );
            if(num_bytes >= 0)
            {
                res.out_string[num_bytes] = 0;
            } else {
                res.tag = NATIVE_ERR_INVALID;
                execvpe_errno = errno;
                switch (execvpe_errno) {
                    case EINTR:
                        res.tag = NATIVE_ERR_INTERRUPT;
                        return res;
                    default:
                        return res;
                }
            }
        }

        res.tag = NATIVE_ERR_SUCCESS;
        res.exit_code = WEXITSTATUS(exit_code);
        return res;
    }
        
    //If running eval, redirect STD output to pipe
    if(is_eval) {
        close(pipe_output_fd[0]);
        dup2(pipe_output_fd[1], STDOUT_FILENO);
    }
    execvpe(executable, exeargs, envstrings);  /* should never return */

    if (!background) {
        /* If execvpe ever returns, an error occurred: */
        close(pipe_fd[0]);

        execvpe_errno = errno;
        write(pipe_fd[1], &execvpe_errno, sizeof(execvpe_errno));
    }

    exit(0);
}