#ifndef COMS_PLATFORM_WIN32_PROCESS
#define COMS_PLATFORM_WIN32_PROCESS

#include <windows.h>
#include <psapi.h>
#include "../../stdlib/Types.h"

// Find all running process ids
int32 process_ids(int32* pids, int32 max_count) {
    int32 bytes_needed = 0;
    int32 max_bytes = max_count * sizeof(int32);

    if (!EnumProcessesA((DWORD *) pids, max_bytes, &bytes_needed)) {
        return -1;
    }

    int32 count = bytes_needed / sizeof(DWORD);
    if (count > max_count) {
        count = max_count;
    }

    return count;
}

#endif
