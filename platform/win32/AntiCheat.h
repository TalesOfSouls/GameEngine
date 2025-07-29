#ifndef COMS_PLATFORM_WIN32_ANTI_CHEAT
#define COMS_PLATFORM_WIN32_ANTI_CHEAT

#include <windows.h>
#include "../../stdlib/Types.h"
#include "../../data/AntiCheat.h"

struct AntiCheatProcessInfo {
    DWORD pid;
    bool scanned;
    bool cheat_detected;
};

// Scan a process's memory for the byte pattern
// helper_memory is a temp buffer that can be used to load running process data
// If the process is larger than the helper memory it will only be loaded partially
bool anti_cheat_memory_scan(
    HANDLE hProcess,
    const AntiCheatSignature* patterns, size_t pattern_count,
    byte* helper_memory, size_t helper_memory_size
) {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    BYTE* addr = (BYTE *) sys_info.lpMinimumApplicationAddress;

    while (addr < (BYTE *) sys_info.lpMaximumApplicationAddress) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) != sizeof(mbi)) {
            break;
        }

        if ((mbi.State == MEM_COMMIT) && (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ))) {
            SIZE_T read_size = (mbi.RegionSize < helper_memory_size) ? mbi.RegionSize : helper_memory_size;
            SIZE_T bytes_read = 0;

            if (ReadProcessMemory(hProcess, addr, helper_memory, read_size, &bytes_read)) {
                for (size_t p = 0; p < pattern_count; ++p) {
                    const AntiCheatSignature& sig = patterns[p];

                    if (sig.pattern_len > bytes_read)
                        continue;

                    for (size_t i = 0; i <= bytes_read - sig.pattern_len; ++i) {
                        if (memcmp(sig.encrypted_pattern, helper_memory + i, sig.pattern_len) == 0) {
                            return true;
                        }
                    }
                }
            }
        }

        addr += mbi.RegionSize;
    }

    return false;
}

// Scan processes using tracking array
void anti_cheat_process_scan(
    AntiCheatSignature* patterns, int32 pattern_count,
    AntiCheatProcessInfo* list, int32 process_count,
    byte* helper_memory,
    size_t helper_memory_size
) {
    for (int32 i = 0; i < process_count; ++i) {
        if (list->pid || list[i].scanned) {
            continue;
        }

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, list[i].pid);
        if (hProcess) {
            // @question Maybe we should pass the decrypted_pattern?
            bool found = anti_cheat_memory_scan(hProcess, patterns, pattern_count, helper_memory, helper_memory_size);
            list[i].cheat_detected = found;
            list[i].scanned = true;

            CloseHandle(hProcess);
        } else {
            list[i].scanned = true;
        }
    }
}

#endif