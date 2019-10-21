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

#include "apolocore.h"

#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

struct native_job_result native_job_status(const int pid, int is_wait)
{
    int status_code;
    int opts = 0;
    if (!is_wait) {
        opts = opts | WNOHANG | WUNTRACED | WCONTINUED;
    }
    
    pid_t result = waitpid(pid, &status_code, opts);

    struct native_job_result res = {NATIVE_ERR_BACKGROUND_UNCHANGED, 0};
    if (result == 0) {
        // Nothing has changed
        return res;
    } else if (result == -1) {
        // Error
        switch (errno) {
            case ECHILD:
                res.tag = NATIVE_ERR_NOTFOUND;
                return res;
            default:
                res.tag = NATIVE_ERR_INVALID;
                return res;
        }
    } else {
        // Default return: error termination (failed)
        res.tag = NATIVE_ERR_BACKGROUND_FAILED;
        res.exit_code = 0;
        if (WIFEXITED(status_code)) {
            // Test if the process exited normally. If this returns false, the process errored out
            res.tag = NATIVE_ERR_BACKGROUND_FINISHED;
            res.exit_code = WEXITSTATUS(status_code);
        } else if (WIFSTOPPED(status_code)) {
            res.tag = NATIVE_ERR_BACKGROUND_SUSPENDED;
        } else if (WIFCONTINUED(status_code)) {
            res.tag = NATIVE_ERR_BACKGROUND_RESUMED;
        }
        return res;
    }
}

static struct native_job_result native_job_signal(const int pid, int signal)
{
    int status = kill(pid, signal);
    if (status < 0) {
        struct native_job_result res = {NATIVE_ERR_INVALID, 0};
        switch (errno) {
            case EPERM:
                res.tag = NATIVE_ERR_PERMISSION;
                return res;
            case ESRCH:
                res.tag = NATIVE_ERR_NOTFOUND;
                return res;
            default:
                return res;
        }
    } else {
        struct native_job_result res = {NATIVE_ERR_SUCCESS, 1};
        return res;
    }
}

struct native_job_result native_job_kill(const int pid, int is_kill)
{
    int signal;
    if (is_kill) {
        signal = SIGKILL;
    } else {
        signal = SIGTERM;
    }
    return native_job_signal(pid, signal);
}

struct native_job_result native_job_set_active(const int pid, int is_suspend)
{
    int signal;
    if (is_suspend) {
        signal = SIGSTOP;
    } else {
        signal = SIGCONT;
    }
    return native_job_signal(pid, signal);
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

#define close_files(out_file, err_file) \
{\
    if(err_file)\
        close(err_file);\
    if(out_file)\
        close(out_file);\
}

struct native_run_result native_setup_proc_out(enum exec_opts_t opts,
    const char *target_file, const char *err_target_file)
{
    struct native_run_result res;
    res.tag = NATIVE_ERR_SUCCESS;

    // Open files before fork so that they can be closed in parent process
    int eval_pipe_fd[2]; int out_target; int out_file = 0;
    
    if (opts & EXEC_OPTS_EVAL) {
        pipe(eval_pipe_fd);
    } else if (target_file) {
        if (opts & EXEC_OPTS_APPEND_TO)
            out_target = open(target_file, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        else
            out_target = open(target_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

        if (out_target < 0) {
            int execvpe_errno = errno;
            switch (execvpe_errno) {
                case EACCES:
                    res.tag = NATIVE_ERR_PERMISSION;
                    return res;
                case EINTR:
                    res.tag = NATIVE_ERR_INTERRUPT;
                    return res;
                case EMFILE: case ENFILE:
                    res.tag = NATIVE_ERR_MAX;
                    return res;
                case ENAMETOOLONG:
                    res.tag = NATIVE_ERR_VARIABLE_SIZE;
                    return res;
                default:
                    return res;
            }
        }
        out_file = out_target;
    } else {
        //Run statement without a target file just goes to stdout
        out_target = STDOUT_FILENO;
    }

    //Set err target
    int err_target; int err_file = 0;
    if (err_target_file) {
        if (opts & EXEC_OPTS_APPEND_ERR)
            err_target = open(err_target_file, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        else
            err_target = open(err_target_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

        if (err_target < 0) {
            int execvpe_errno = errno;
            switch (execvpe_errno) {
                case EACCES:
                    res.tag = NATIVE_ERR_PERMISSION;
                    if(out_file)
                        close(out_file);
                    return res;
                case EINTR:
                    res.tag = NATIVE_ERR_INTERRUPT;
                    if(out_file)
                        close(out_file);
                    return res;
                case EMFILE: case ENFILE:
                    res.tag = NATIVE_ERR_MAX;
                    if(out_file)
                        close(out_file);
                    return res;
                case ENAMETOOLONG:
                    res.tag = NATIVE_ERR_VARIABLE_SIZE;
                    if(out_file)
                        close(out_file);
                    return res;
                default:
                    if(out_file)
                        close(out_file);
                    return res;
            }
        }
        err_file = err_target;
    } else {
        //Run statement without a target file just goes to stdout
        err_target = STDERR_FILENO;
    }

    /* Create pipe to communicate execvpe failure in the child */
    int err_pipe_fd[2];
    if (!(opts & EXEC_OPTS_BG))
        pipe(err_pipe_fd);

    //Create process that reads and prepares output
    pid_t fork_res = fork();
    if (fork_res < 0) {
        res.tag = NATIVE_ERR_FORKFAILED;
        close_files(out_file, err_file);
        return res;
    }

    if (fork_res != 0) {

        if (!(opts & EXEC_OPTS_BG)) {
            int execvpe_errno = 0;
            close(err_pipe_fd[1]);
            read(err_pipe_fd[0], &execvpe_errno, sizeof(execvpe_errno));

            switch (execvpe_errno) {
            case 0:  // success
                break;
            case ENOMEM: case EAGAIN: case ENOSYS:
                res.tag = NATIVE_ERR_FORKFAILED;
                close_files(out_file, err_file);
                return res;
            case ENOENT:
                res.tag = NATIVE_ERR_NOTFOUND;
                close_files(out_file, err_file);
                return res;
            // TODO treat other exec errors
            default:
                close_files(out_file, err_file);
                return res;
            }

            int exit_code;
            waitpid(fork_res, &exit_code, 0);

            // Process is finished. Close files
            close_files(out_file, err_file);

            // Get output from process
            if (opts & EXEC_OPTS_EVAL) {
                close(eval_pipe_fd[1]);
                // Read bytes
                int num_bytes = read(eval_pipe_fd[0] , res.out_string , EVAL_BUFFER_SIZE);
                if (num_bytes >= 0) {
                    res.out_string[num_bytes] = 0;
                }
                else {
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

            // Get exit code from last process
            res.tag = NATIVE_ERR_SUCCESS;
            res.exit_code = WEXITSTATUS(exit_code);
        } else {
            res.tag = NATIVE_ERR_BACKGROUND_SUCCESS;
            res.pid = fork_res;
        }
        return res;
    }

    // We're the child. Prepare to start processes
    if (!(opts & EXEC_OPTS_BG)) {
        close(err_pipe_fd[0]);
        res.pipe_info.err_pipe_fd = err_pipe_fd[1];
    }

    if (opts & EXEC_OPTS_EVAL) {
        close(eval_pipe_fd[0]);
        out_target = eval_pipe_fd[1];
    }

    if (opts & EXEC_OPTS_ERR_TO_OUT) {
        res.pipe_info.error_fd = out_target;
    } else {
        res.pipe_info.error_fd = err_target;
    }
    
    if (opts & EXEC_OPTS_OUT_TO_ERR) {
        res.pipe_info.write_fd = err_target;
    } else {
        res.pipe_info.write_fd = out_target;
    }
    res.tag = NATIVE_ERR_IN_EXECUTE;

    return res;
}

struct native_run_result native_execute(
    const char *executable, const char **exeargs, const char **envstrings,
    enum exec_opts_t opts, struct native_run_result res, int index, const char *source_file)
{
    res.tag = NATIVE_ERR_IN_EXECUTE;
    /* Execute setup run at beginning of recursive run */
    const char **env = envstrings;
    char **parent_env = environ;

    for (; *env; ++env);  /* go to end of array */

    /* Copy parent env to child env array */
    for (; *parent_env; ++parent_env, ++env)
        *env = *parent_env;
    *env = NULL;

    //Make pipe
    int new_pipe[2];
    pipe(new_pipe);

    /* Fork the process to avoid the script being replaced by execvp */
    pid_t fork_res = fork();
    if (fork_res < 0) {
        int execvpe_errno = 0;
        execvpe_errno = errno;
        //Errors in execute_pipe need to message the proc_out listener to clean up run execution
        write(res.pipe_info.err_pipe_fd, &execvpe_errno, sizeof(execvpe_errno));
        exit(0);
    }

    //Fork into two processes
    if (fork_res != 0) {
        //We're the parent. Start new process
        dup2(res.pipe_info.write_fd, STDOUT_FILENO);

        //Pipe to next process if this is not the first process
        if (index > 0) {
            close(new_pipe[1]);
            dup2(new_pipe[0], STDIN_FILENO);
        }
        else if (source_file) {
            int source;
            source = open(source_file, O_RDONLY);
            if (source < 0) {
                int execvpe_errno = errno;
                switch (execvpe_errno) {
                    case EACCES:
                        res.tag = NATIVE_ERR_PERMISSION;
                        return res;
                    case EINTR:
                        res.tag = NATIVE_ERR_INTERRUPT;
                        return res;
                    case EMFILE: case ENFILE:
                        res.tag = NATIVE_ERR_MAX;
                        return res;
                    case ENAMETOOLONG:
                        res.tag = NATIVE_ERR_VARIABLE_SIZE;
                        return res;
                    case ENOENT: case ENOTDIR:
                        res.tag = NATIVE_ERR_FILE_NOTFOUND;
                        return res;
                    default:
                        return res;
                }
            }
            //Set input of this first process to the source file
            dup2(source, STDIN_FILENO);
        }

        dup2(res.pipe_info.error_fd, STDERR_FILENO);

        //Start process
        execvpe(executable, exeargs, envstrings);  /* should never return */

        if (!(opts & EXEC_OPTS_BG)) {
            // If execvpe ever returns, an error occurred:
            int execvpe_errno = 0;
            execvpe_errno = errno;
            write(res.pipe_info.err_pipe_fd, &execvpe_errno, sizeof(execvpe_errno));
        }

        exit(0);
    } else {
        close(new_pipe[0]);
        res.pipe_info.write_fd = new_pipe[1];

        return res;
    }
}

struct native_run_result native_execute_begin(struct native_run_result proc,
    enum exec_opts_t opts)
{
    exit(0);
}