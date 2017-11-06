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

#include <string.h>
#include <windows.h>

const char *apolocore_os = "win";

int native_chdir(const char *dir)
{
    return SetCurrentDirectory(dir) != 0;
}

void native_curdir(char *dir)
{
    GetCurrentDirectory(512, dir);
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

int native_run(
    const char *executable, const char **exeargs, const char **envstrings)
{
    char cmdline[1024];
    char env[4096];
    char *env_ptr = env;
    char *parent_env = GetEnvironmentStrings();
    char *parent_env_ptr = parent_env;
    STARTUPINFO suinfo;
    PROCESS_INFORMATION pinfo;

    strcpy(cmdline, "\"");
    strcat(cmdline, executable);
    strcat(cmdline, ".exe");
    strcat(cmdline, "\" ");

    ++exeargs;
    for (; *exeargs != NULL; ++exeargs) {
        strcat(cmdline, "\"");
        strcat(cmdline, *exeargs);
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

    memset(&suinfo, 0, sizeof(suinfo));
    suinfo.cb = sizeof(suinfo);

    memset(&pinfo, 0, sizeof(pinfo));

    CreateProcess(
        NULL, cmdline, NULL, NULL, FALSE, 0, env,
        NULL, &suinfo, &pinfo);

    return WaitForSingleObject(pinfo.hProcess, INFINITE) == WAIT_OBJECT_0;
}
