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

#include <string.h>
#include <windows.h>
#include <shlwapi.h>

#include <stdio.h>

const char *apolocore_os = "win";

int native_chdir(const char *dir)
{
    return SetCurrentDirectory(dir) != 0;
}
static int append_filename_to_path(const char *orig, const char *dest, char* dest_ex)
{
    strcpy(dest_ex, dest); 
    if (!PathIsFileSpec(orig) || !PathIsFileSpec(dest)) 
        return 1;
    if (PathIsDirectory(dest)) {
        char *orig_filename = PathFindFileName(orig);
        if (strlen(orig_filename) + strlen(dest) >= MAX_PATH)
            return 0;
        PathAppend(dest_ex, orig_filename);
    }
    return 1;
}

static int native_file_operation(const char *orig, const char *dest, UINT operation) {
    
    char dest_ex[MAX_PATH] = "";
    char orig_ex[MAX_PATH] = "";
    strcpy(orig_ex, orig);
    strcpy(dest_ex, dest);
    if (!append_filename_to_path(orig, dest, dest_ex))
        return 0;
    strncat(dest_ex, "\0", MAX_PATH); strncat(orig_ex, "\0", MAX_PATH);
    
    SHFILEOPSTRUCT file_op = { 0 };
    file_op.hwnd = 0;
    file_op.wFunc = operation;
    file_op.pFrom = orig_ex;
    file_op.pTo = dest_ex;
    file_op.fFlags = FOF_NOCONFIRMMKDIR |
        FOF_NOCONFIRMATION |
        FOF_NOERRORUI |
        FOF_SILENT;

    return SHFileOperation(&file_op) == 0;
}

int native_copy(const char *orig, const char *dest)
{
    return native_file_operation(orig, dest, FO_COPY);
}

void native_curdir(char *dir)
{
    GetCurrentDirectory(512, dir);
}

int native_exists(const char *path)
{
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

int native_fillentryarray(lua_State *L, const char *dir)
{
    WIN32_FIND_DATA file_info;
    HANDLE hfind;
    char dir_pattern[512];
    int i = 1;

    strcpy(dir_pattern, dir);
    strcat(dir_pattern, "\\*");

    hfind = FindFirstFile(dir_pattern, &file_info);

    if (hfind == INVALID_HANDLE_VALUE)
        return 0;

    do {
        const char *type;

        if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            type = "dir";
        else
            type = "file";

        insert_direntry(L, i, file_info.cFileName, type);
        ++i;
    } while (FindNextFile(hfind, &file_info) != 0);

    FindClose(hfind);

    return 1;
}

int native_mkdir(const char *dir)
{
    SECURITY_ATTRIBUTES sec;
    sec.nLength = sizeof(sec);
    sec.lpSecurityDescriptor = NULL;
    sec.bInheritHandle = FALSE;

    return CreateDirectory(dir, &sec) != 0;
}

int native_move(const char *orig, const char *dest)
{
    return native_file_operation(orig, dest, FO_MOVE);
}

int native_rmdir(const char *dir)
{
    return RemoveDirectory(dir);
}

struct native_run_result native_setup_proc_out(enum exec_opts_t opts,
    const char *target_file, const char *err_target_file)
{
    struct native_run_result res;

    //Prepare the eval pipe
    HANDLE write_handle, pipe_eval_rd;
    if (opts & EXEC_OPTS_EVAL) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        
        if (! CreatePipe(&pipe_eval_rd, &write_handle, &sa, 0)) {
            CloseHandle(write_handle);
            CloseHandle(pipe_eval_rd);
            res.tag = NATIVE_ERR_PIPE_FAILED;
            return res;
        }
        if (! SetHandleInformation(pipe_eval_rd, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(write_handle);
            CloseHandle(pipe_eval_rd);
            res.tag = NATIVE_ERR_PIPE_FAILED;
            return res;
        }
    } else if (target_file) {
        SECURITY_ATTRIBUTES file_sa; 
        file_sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
        file_sa.bInheritHandle = TRUE; 
        file_sa.lpSecurityDescriptor = NULL;

        int creation_rights = CREATE_ALWAYS;
        DWORD access_rights = FILE_WRITE_DATA;
        if (opts & EXEC_OPTS_APPEND_TO) {
            access_rights = FILE_APPEND_DATA;
            creation_rights = OPEN_ALWAYS;
        }

        write_handle = CreateFile(target_file,
                    access_rights, 0, &file_sa,
                    creation_rights, FILE_ATTRIBUTE_NORMAL, // Create_always?
                    NULL);
        res.pipe_info.file_handles[1] = write_handle;
        
        if (write_handle == INVALID_HANDLE_VALUE) {
            switch (GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                res.tag = NATIVE_ERR_FILE_NOTFOUND;
                return res;
            default:
                res.tag = NATIVE_ERR_INVALID;
                return res;
            }
        }

        if (opts & EXEC_OPTS_APPEND_TO) {
            SetFilePointer(write_handle, 0, NULL, FILE_END);
        }
    } else {
        write_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    
    HANDLE error_handle;
    if (err_target_file) {
        SECURITY_ATTRIBUTES file_sa;
        file_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        file_sa.bInheritHandle = TRUE;
        file_sa.lpSecurityDescriptor = NULL;

        int creation_rights;
        DWORD access_rights;
        if (opts & EXEC_OPTS_APPEND_ERR) {
            access_rights = FILE_APPEND_DATA;
            creation_rights = OPEN_ALWAYS;
        } else {
            creation_rights = CREATE_ALWAYS;
            access_rights = FILE_WRITE_DATA;
        }
        
        error_handle = CreateFile(err_target_file,
                    access_rights, 0, &file_sa,
                    creation_rights, FILE_ATTRIBUTE_NORMAL,
                    NULL);

        if (error_handle == INVALID_HANDLE_VALUE) {
            switch (GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                res.tag = NATIVE_ERR_FILE_NOTFOUND;
                return res;
            default:
                res.tag = NATIVE_ERR_INVALID;
                return res;
            }
        }
        res.pipe_info.file_handles[2] = error_handle;
    }
    else if (opts & EXEC_OPTS_ERR_TO_OUT) {
        error_handle = res.pipe_info.write_handle;
    } else {
        error_handle = GetStdHandle(STD_ERROR_HANDLE);
    }

    if (opts & EXEC_OPTS_OUT_TO_ERR) {
        res.pipe_info.write_handle = error_handle;
    } else {
        res.pipe_info.write_handle = write_handle;
    }
    if (opts & EXEC_OPTS_ERR_TO_OUT) {
        res.pipe_info.error_handle = write_handle;
    } else {
        res.pipe_info.error_handle = error_handle;
    }
    res.pipe_info.read_handle = pipe_eval_rd;
    res.pipe_info.final_process = NULL;
    res.tag = NATIVE_ERR_PROCESS_RUNNING;
    return res;
}

// Returning pipe_fail error from execute_in_pipe
struct native_run_result native_execute(
    const char *executable, const char **exeargs, const char **envstrings,
    enum exec_opts_t opts, struct native_run_result res, int index,
    const char *source_file)
{
    char cmdline[1024];
    char env[4096];
    char *env_ptr = env;
    char *parent_env = GetEnvironmentStrings();
    char *parent_env_ptr = parent_env;

    cmdline[0] = '\0';
    strcpy(cmdline, "\"");
    strcat(cmdline, executable);
    strcat(cmdline, "\" ");

    for (int i=1; exeargs[i] != NULL && i<32; i++) {
        strcat(cmdline, "\"");
        strcat(cmdline, exeargs[i]);
        strcat(cmdline, "\" ");
    }

    // Copy envstrings to env
    for (; *envstrings; ++envstrings, ++env_ptr) {
        for (const char *cur = *envstrings; *cur; ++cur, ++env_ptr)
            *env_ptr = *cur;
        *env_ptr = '\0';
    }

    // Copy parent_env to env until current and next characters are both zero
    for (; !(parent_env_ptr[0] == '\0' && parent_env_ptr[1] == '\0'); ++parent_env_ptr, ++env_ptr)
        *env_ptr = *parent_env_ptr;
    *env_ptr = '\0';
    ++env_ptr;
    *env_ptr = '\0';

    FreeEnvironmentStrings(parent_env);
    
    //Create new startup info
    STARTUPINFO suinfo;
    memset(&suinfo, 0, sizeof(suinfo));
    suinfo.cb = sizeof(suinfo);

    suinfo.hStdError = res.pipe_info.error_handle;
    suinfo.hStdOutput = res.pipe_info.write_handle;

    //Make pipe
    SECURITY_ATTRIBUTES sa; 
    sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL; 

    //Set input depending on if this is/isn't the first process in the pipe
    if (index > 0) {
        HANDLE pipe_out_rd, pipe_out_wr;
        if (! CreatePipe(&pipe_out_rd, &pipe_out_wr, &sa, 0)) {
            CloseHandle(pipe_out_wr);
            CloseHandle(pipe_out_rd);
            res.tag = NATIVE_ERR_PIPE_FAILED;
            return res;
        }
        suinfo.hStdInput = pipe_out_rd;
        res.pipe_info.write_handle = pipe_out_wr;
    } else {
        //Prepare the process's input
        if (source_file) {
            SECURITY_ATTRIBUTES file_sa; 
            file_sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
            file_sa.bInheritHandle = TRUE; 
            file_sa.lpSecurityDescriptor = NULL; 
            suinfo.hStdInput = CreateFile(source_file,
                            GENERIC_READ, 0, &file_sa,
                            OPEN_EXISTING, FILE_ATTRIBUTE_READONLY,
                            NULL);
            res.pipe_info.file_handles[0] = suinfo.hStdInput;

            if ( suinfo.hStdInput == INVALID_HANDLE_VALUE) {
                switch (GetLastError()) {
                case ERROR_FILE_NOT_FOUND:
                    res.tag = NATIVE_ERR_FILE_NOTFOUND;
                    return res;
                default:
                    res.tag = NATIVE_ERR_INVALID;
                    return res;
                }
            }
        } else {
            //Run statement without a target file just goes to stdout
            suinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }
    }
    suinfo.dwFlags = STARTF_USESTDHANDLES;

    //Create new process info
    PROCESS_INFORMATION pinfo;
    //Return whether creating process succeeds
    if (!CreateProcess(NULL, cmdline, NULL, NULL,
        TRUE, 0, env, NULL, &suinfo, &pinfo)) {
        switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
            res.tag = NATIVE_ERR_NOTFOUND;
            return res;
        default:
            return res;
        }
    }

    // If no final process has been set, that means THIS is the final process
    if (res.pipe_info.final_process == NULL) {
        res.pipe_info.final_process = pinfo.hProcess;
    }

    return res;
}

struct native_run_result native_execute_begin(struct native_run_result res,
    enum exec_opts_t opts)
{
    if (!(opts & EXEC_OPTS_BG)) {
        WaitForSingleObject(res.pipe_info.final_process, INFINITE);
        res.tag = NATIVE_ERR_SUCCESS;
        res.exit_code = 0;

        //Get output from process
        if (opts & EXEC_OPTS_EVAL) {
            DWORD bytes_read;
            
            ReadFile(res.pipe_info.read_handle, res.out_string, EVAL_BUFFER_SIZE, &bytes_read, NULL);
            res.out_string[bytes_read] = 0;
            CloseHandle(res.pipe_info.read_handle);
        }

        //Get exit code from hProcess of last process
        GetExitCodeProcess(res.pipe_info.final_process, (PDWORD) &res.exit_code);
        CloseHandle(res.pipe_info.final_process);
    } else {
        res.tag = NATIVE_ERR_BACKGROUND_SUCCESS;
    }

    //Close all files
    for(int i=0; i<3; i++) {
        CloseHandle(res.pipe_info.file_handles[i]);
        res.pipe_info.file_handles[i] = NULL;
    }

    return res;
}