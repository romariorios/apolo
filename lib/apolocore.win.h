#include <windows.h>

struct native_pipe_info
{
    HANDLE write_handle;
    HANDLE error_handle;
    HANDLE read_handle;
    HANDLE final_process;
    HANDLE file_handles[3];
};