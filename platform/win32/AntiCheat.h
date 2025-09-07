#ifndef COMS_PLATFORM_WIN32_ANTI_CHEAT
#define COMS_PLATFORM_WIN32_ANTI_CHEAT

#include <windows.h>
#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../data/AntiCheat.h"

struct AntiCheatProcessInfo {
    uint32 pid;
    bool scanned;
    bool cheat_detected;
    bool is_active;
};

// Scan a process's memory for the byte pattern
// helper_memory is a temp buffer that can be used to load running process data
// If the process is larger than the helper memory it will only be loaded partially
static
bool anti_cheat_memory_scan(
    HANDLE hProcess,
    const AntiCheatSignature* __restrict patterns, size_t pattern_count,
    byte* __restrict helper_memory, size_t helper_memory_size
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
                    const AntiCheatSignature* sig = &patterns[p];
                    if (sig->length > bytes_read) {
                        continue;
                    }

                    // @bug in theory our signature is split across 2 read chunks
                    if (byte_contains(helper_memory, bytes_read, sig->encrypted_pattern, sig->length)) {
                        return true;
                    }
                }
            }
        }

        addr += mbi.RegionSize;
    }

    return false;
}

// Scan processes using tracking array
inline
void anti_cheat_process_scan(
    const AntiCheatSignature* __restrict patterns, int32 pattern_count,
    AntiCheatProcessInfo* __restrict list, int32 process_count,
    byte* __restrict helper_memory, size_t helper_memory_size
) {
    for (int32 i = 0; i < process_count; ++i) {
        if (list[i].scanned) {
            // @todo how to remove no longer active processes?
            // maybe handle that outside in regular intervals... check all running processes
            continue;
        }

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, list[i].pid);
        list[i].scanned = true;
        if (hProcess) {
            // @question Maybe we should pass the decrypted_pattern?
            list[i].cheat_detected = anti_cheat_memory_scan(hProcess, patterns, pattern_count, helper_memory, helper_memory_size);

            CloseHandle(hProcess);
        }
    }
}

#endif