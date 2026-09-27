#pragma once
#ifndef COMS_PLATFORM_WIN32_DRM
#define COMS_PLATFORM_WIN32_DRM

#include "../../stdlib/Stdlib.h"

#include <windows.h>
#include <psapi.h>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

#ifndef ProcessBasicInformation
    #define ProcessBasicInformation 0
#endif

#ifndef ThreadHideFromDebugger
    #define ThreadHideFromDebugger 0x11
#endif

inline
bool drm_prevent_debugger_attach() NO_EXCEPT
{
    return NtSetInformationThread(
        GetCurrentThread(),
        (THREADINFOCLASS) ThreadHideFromDebugger,
        NULL,
        0
    ) == 0;
}

#define ProcessDebugPort 7
#define ProcessDebugObjectHandle 0x1E
#define ProcessDebugFlags 0x1F
bool drm_debug_object_present() NO_EXCEPT
{
    HANDLE self = GetCurrentProcess();
    ULONG_PTR port = 0;
    if (NtQueryInformationProcess(
            self, (PROCESSINFOCLASS) ProcessDebugPort,
            &port, sizeof(port), NULL
        ) == 0 && port != 0
    ) {
        return true;
    }

    HANDLE dbg = NULL;
    if (NtQueryInformationProcess(
            self, (PROCESSINFOCLASS) ProcessDebugObjectHandle,
            &dbg, sizeof(dbg), NULL
        ) == 0 && dbg != NULL
    ) {
        return true;
    }

    // 0 == debugger attached
    DWORD flags = 0;
    if (NtQueryInformationProcess(
            self, (PROCESSINFOCLASS) ProcessDebugFlags,
            &flags, sizeof(flags), NULL
        ) == 0 && flags == 0
    ) {
        return true;
    }

    return false;
}

bool drm_is_being_debugged() NO_EXCEPT
{
    PROCESS_BASIC_INFORMATION pbi = {0};
    ULONG ret_length = 0;

    const NTSTATUS status = NtQueryInformationProcess(
        GetCurrentProcess(),
        (PROCESSINFOCLASS) ProcessBasicInformation,
        &pbi, sizeof(pbi),
        &ret_length
    );

    if (status != 0) {
        return false;
    }

    const BYTE* const pPeb = (BYTE *) pbi.PebBaseAddress;

    // BeingDebugged
    return pPeb[2] != 0;
}

bool drm_has_hardware_breakpoints() NO_EXCEPT
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return false;
    }

    return (ctx.Dr0 | ctx.Dr1 | ctx.Dr2 | ctx.Dr3) != 0;
}

// Example: process_names = { "x64dbg.exe", "cheatengine-x86_64.exe", "ollydbg.exe" };
bool drm_check_process_name(const utf16** process_names, int32 count) NO_EXCEPT
{
    DWORD processes[1024];
    DWORD cb_needed;
    if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) {
        return false;
    }

    const uint32 n = cb_needed >> div_pow2(sizeof(DWORD));
    for (uint32 i = 0; i < n; ++i) {
        if (processes[i] == 0) {
            continue; // System Idle Process
        }

        HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
        if (!process_handle) {
            continue;
        }

        wchar_t image_path[PATH_MAX_LENGTH];
        DWORD len = ARRAY_COUNT(image_path);

        if (QueryFullProcessImageNameW(process_handle, 0, image_path, &len)) {
            const wchar_t* base = image_path;
            for (DWORD k = 0; k < len; ++k) {
                if (image_path[k] == L'\\' || image_path[k] == L'/') {
                    base = image_path + k + 1;
                }
            }

            for (int32 j = 0; j < count; ++j) {
                if (str_contains(base, process_names[j]) == 0) {
                    CloseHandle(process_handle);
                    return true;
                }
            }
        }

        CloseHandle(process_handle);
    }

    return false;
}

struct WindowTitleSearchContext {
    const wchar_t** titles;
    int32 count;
    bool found;
};

static
BOOL CALLBACK drm_enum_windows_callback(HWND hWnd, LPARAM lParam) {
    WindowTitleSearchContext* const ctx = (WindowTitleSearchContext *) lParam;

    wchar_t window_title[256];
    GetWindowTextW(hWnd, window_title, ARRAY_COUNT(window_title) - 1);
    window_title[255] = L'\0';

    for (int32 i = 0; i < ctx->count; ++i) {
        if (str_contains(window_title, ctx->titles[i]) == 0) {
            ctx->found |= true;
            return true;
        }
    }

    return false;
}

inline
bool drm_check_window_title(const wchar_t** window_titles, int32 count) NO_EXCEPT
{
    WindowTitleSearchContext ctx = { window_titles, count, false };
    EnumWindows(drm_enum_windows_callback, (LPARAM) &ctx);

    return ctx.found;
}

#include "../../data/DRM.h"

struct DRMProcessInfo {
    uint32 pid;
    bool scanned;
    bool code_detected;
    bool is_active;
};

// Scan a process's memory for the byte pattern
// helper_memory is a temp buffer that can be used to load running process data
// If the process is larger than the helper memory it will only be loaded partially
// Ideal to scan malicious processes for evil patterns or scan own process for code injection
static inline
bool drm_memory_scan(
    HANDLE hProcess,
    const DRMDecodedSignature* __restrict patterns, size_t pattern_count,
    size_t max_pattern_len,
    byte* __restrict helper_memory, size_t helper_memory_size
) NO_EXCEPT
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    BYTE* addr = (BYTE *) sys_info.lpMinimumApplicationAddress;
    BYTE* const max_addr = (BYTE *) sys_info.lpMaximumApplicationAddress;
    const size_t overlap = (max_pattern_len > 0) ? max_pattern_len - 1 : 0;

    while (addr < max_addr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) != sizeof(mbi)) {
            break;
        }

        BYTE* const region_base = (BYTE *) mbi.BaseAddress;
        BYTE* const region_end = region_base + mbi.RegionSize;

        const bool scannable =
            (mbi.State == MEM_COMMIT)
            && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
            && (mbi.Protect & (
                    PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY
                    | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY
                )
            );

        if (scannable) {
            BYTE* chunk = region_base;
            while (chunk < region_end) {
                const size_t remaining = (size_t)(region_end - chunk);
                const size_t want = (remaining < helper_memory_size) ? remaining : helper_memory_size;
                size_t bytes_read = 0;

                if (ReadProcessMemory(hProcess, chunk, helper_memory, want, &bytes_read)
                    && bytes_read
                ) {
                    for (size_t p = 0; p < pattern_count; ++p) {
                        if (patterns[p].length > bytes_read) {
                            continue;
                        }

                        if (byte_contains(
                            helper_memory,
                            bytes_read,
                            patterns[p].data, patterns[p].length)
                        ) {
                            return true;
                        }
                    }
                }

                // unreadable page, skip region
                if (bytes_read == 0) {
                    break;
                }

                // partial read
                if (bytes_read < want) {
                    chunk += bytes_read;
                    continue;
                }

                // guard against non-progress
                if (want <= overlap) {
                    break;
                }

                // keep overlap for split signatures
                chunk += (want - overlap);
            }
        }

        // guard against stall
        if (mbi.RegionSize == 0) {
            break;
        }

        addr = region_end;
    }

    return false;
}

// Scan processes using tracking array
inline
void drm_process_scan(
    const DRMDecodedSignature* __restrict patterns, size_t pattern_count, size_t max_pattern_len,
    DRMProcessInfo* __restrict list, int32 process_count,
    byte* __restrict helper_memory, size_t helper_memory_size
) NO_EXCEPT
{
    for (int32 i = 0; i < process_count; ++i) {
        if (list[i].scanned || list[i].pid == 0) {
            continue;
        }

        HANDLE hProcess = OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
            false,
            list[i].pid
        );
        list[i].scanned = true;

        if (hProcess) {
            list[i].code_detected = drm_memory_scan(
                hProcess, patterns, pattern_count, max_pattern_len,
                helper_memory, helper_memory_size
            );

            CloseHandle(hProcess);
        }
    }
}

static uint32 drm_hash(const byte* base, size_t len) NO_EXCEPT
{
    // FNV-1a
    uint32 h = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        h ^= base[i]; h *= 16777619u;
    }

    return h;
}

// Resolve the module's .text range from its PE headers, hash it.
bool drm_text_integrity_ok(uint32 expected) NO_EXCEPT
{
    const HMODULE mod = GetModuleHandleW(NULL);
    const BYTE* base = (const BYTE *) mod;
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER *) base;
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS *) (base + dos->e_lfanew);

    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 5) == 0) {
            return drm_hash(base + sec[i].VirtualAddress, sec[i].Misc.VirtualSize) == expected;
        }
    }

    return true;
}

#endif