#ifndef COMS_PLATFORM_WIN32_PROCESS
#define COMS_PLATFORM_WIN32_PROCESS

#include <windows.h>
#include <psapi.h>
#include "../../stdlib/Stdlib.h"
#include "../../compiler/CompilerUtils.h"

// Find all running process ids
inline
int32 process_id_enum(uint32* pids, int32 max_count) NO_EXCEPT
{
    DWORD bytes_needed = 0;
    DWORD max_bytes = (DWORD) (max_count * sizeof(DWORD));

    // Call EnumProcesses with correct types
    if (!EnumProcesses((DWORD *) pids, max_bytes, &bytes_needed)) {
        return -1; // failure
    }

    int32 count = (int32) bytes_needed / sizeof(DWORD);
    if (count > max_count) {
        count = max_count;
    }

    return count;
}

FORCE_INLINE
int64 process_id_get() NO_EXCEPT
{
    return GetCurrentProcessId();
}

inline
bool is_window_active(int64 process_id = 0) NO_EXCEPT
{
    HWND foreground = GetForegroundWindow();

    DWORD pid;
    GetWindowThreadProcessId(foreground, &pid);

    if (process_id == 0) {
        process_id = (int64) GetCurrentProcessId();
    }

    return (int64) pid == process_id;
}

#endif
