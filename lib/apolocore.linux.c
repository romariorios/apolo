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

struct native_linux_run_result
{
    enum native_err tag;
    int exit_code;
    char out_string[EVAL_BUFFER_SIZE];

    int write_fd;
    int input_fd;
    int eval_pipe_fd;
    int err_pipe_fd;

    char padding[32 - 4*sizeof(int)];
};

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

void native_setup_proc_out(enum exec_opts_t opts, struct native_run_result *proc)
{
    struct native_linux_run_result *res = (struct native_linux_run_result*) proc;
    res->tag = NATIVE_ERR_SUCCESS;

    int err_pipe_fd[2]; int eval_pipe_fd[2];
    /* Create pipe to communicate execvpe failure in the child */
    if (!(opts & EXEC_OPTS_BG))
        pipe(err_pipe_fd);
    
    if(opts & EXEC_OPTS_EVAL)
        pipe(eval_pipe_fd);
    else
        res->write_fd = STDOUT_FILENO;

    //Create process that reads and prepares output
    pid_t fork_res = fork();
    if (fork_res < 0) {
        res->tag = NATIVE_ERR_FORKFAILED;
        return;
    }

    if(fork_res != 0) {

        if(!(opts & EXEC_OPTS_BG)) {
            int execvpe_errno = 0;
            close(err_pipe_fd[1]);
            read(err_pipe_fd[0], &execvpe_errno, sizeof(execvpe_errno));

            switch (execvpe_errno) {
            case 0:  // success
                break;
            case ENOMEM: case EAGAIN: case ENOSYS:
                res->tag = NATIVE_ERR_FORKFAILED;
                return;
            case ENOENT:
                res->tag = NATIVE_ERR_NOTFOUND;
                return;
            // TODO treat other exec errors
            default:
                return;
            }

            int exit_code;
            waitpid(fork_res, &exit_code, 0);

            //Get output from process
            if(opts & EXEC_OPTS_EVAL) {
                close(eval_pipe_fd[1]);
                //Read bytes
                int num_bytes = read(eval_pipe_fd[0] , res->out_string , EVAL_BUFFER_SIZE );
                if(num_bytes >= 0)
                {
                    res->out_string[num_bytes] = 0;
                }
                else {
                    res->tag = NATIVE_ERR_INVALID;
                    execvpe_errno = errno;
                    switch (execvpe_errno) {
                    case EINTR:
                        res->tag = NATIVE_ERR_INTERRUPT;
                        return;
                    default:
                        return;
                    }
                }
            }

            //Get exit code from last process
            res->tag = NATIVE_ERR_SUCCESS;
            res->exit_code = WEXITSTATUS(exit_code);

            //Return output from here to preserve variables declared in this scope
            return;
        } else {
            res->tag = NATIVE_ERR_BACKGROUND_SUCCESS;
            return;
        }
    }

    // We're the child. Prepare to start processes
    if (!(opts & EXEC_OPTS_BG)) {
        close(err_pipe_fd[0]);
        res->err_pipe_fd = err_pipe_fd[1];
    }
    if(opts & EXEC_OPTS_EVAL) {
        close(eval_pipe_fd[0]);
        res->write_fd = eval_pipe_fd[1];
    }
    res->tag = NATIVE_ERR_PROCESS_RUNNING;

    return;
}

void native_execute(
    const char *executable, const char **exeargs, const char **envstrings,
    enum exec_opts_t opts, struct native_run_result *prev_proc, int index, const char *source_file)
{
    /* Execute setup run at beginning of recursive run */
    const char **env = envstrings;
    char **parent_env = environ;

    for (; *env; ++env);  /* go to end of array */

    /* Copy parent env to child env array */
    for (; *parent_env; ++parent_env, ++env)
        *env = *parent_env;
    *env = NULL;

    //Carry on final_process and read_handle from previous process
    struct native_linux_run_result *res = (struct native_linux_run_result*) prev_proc;

    //Make pipe
    int new_pipe[2];
    pipe(new_pipe);

    pid_t fork_res = fork();
    if (fork_res < 0) {
        int execvpe_errno = 0;
        execvpe_errno = errno;
        //Errors in execute_pipe need to message the proc_out listener to clean up run execution
        write(res->err_pipe_fd, &execvpe_errno, sizeof(execvpe_errno));
        exit(0);
    }

    //Fork into two processes
    if(fork_res != 0) {
        //We're the parent. Start new process
        dup2(res->write_fd, STDOUT_FILENO);

        //Pipe to next process this is NOT the last process
        if(index > 0) {
            close(new_pipe[1]);
            dup2(new_pipe[0], STDIN_FILENO);
        }
        else {
            //If there is some io redirection here, do it!
            //dup2(source_file, STDOUT_FILENO);
        }

        //Start process
        execvpe(executable, exeargs, envstrings);  /* should never return */

        if (!(opts & EXEC_OPTS_BG)) {
            // If execvpe ever returns, an error occurred:
            int execvpe_errno = 0;
            execvpe_errno = errno;
            write(res->err_pipe_fd, &execvpe_errno, sizeof(execvpe_errno));
        }

        exit(0);
    } else {
        close(new_pipe[0]);
        res->write_fd = new_pipe[1];

        return;
    }
}

void native_execute_begin(struct native_run_result *proc,
    enum exec_opts_t opts)
{
    exit(0);
}