#ifndef COMS_PLATFORM_WIN32_DRM
#define COMS_PLATFORM_WIN32_DRM

#include <windows.h>
#include <psapi.h>
#include <winternl.h>
#include "../../stdlib/Stdlib.h"

#ifndef ProcessBasicInformation
    #define ProcessBasicInformation 0
#endif

#ifndef ThreadHideFromDebugger
    #define ThreadHideFromDebugger 0x11
#endif

typedef NTSTATUS (WINAPI *pNtQueryInformationProcess)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

typedef NTSTATUS (WINAPI *pNtSetInformationThread)(
    HANDLE ThreadHandle,
    ULONG ThreadInformationClass,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength
);

inline
bool drm_prevent_debugger_attach() NO_EXCEPT
{
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }

    pNtSetInformationThread NtSetInformationThread = (pNtSetInformationThread)
        GetProcAddress(ntdll, "NtSetInformationThread");

    if (!NtSetInformationThread) {
        return false;
    }

    return NtSetInformationThread(GetCurrentThread(), ThreadHideFromDebugger, nullptr, 0) == 0;
}

bool drm_is_being_debugged() NO_EXCEPT
{
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }

    pNtQueryInformationProcess NtQueryInformationProcess = (pNtQueryInformationProcess)
        GetProcAddress(ntdll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) {
        return false;
    }

    PROCESS_BASIC_INFORMATION pbi = {};
    ULONG returnLength = 0;

    const NTSTATUS status = NtQueryInformationProcess(
        GetCurrentProcess(),
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),
        &returnLength
    );

    if (status != 0) {
        return false;
    }

    // Read BeingDebugged flag from PEB
    const BYTE* const pPeb = (BYTE *) pbi.PebBaseAddress;

    return pPeb[2] != 0;
}

// Example: process_names = { "x64dbg.exe", "cheatengine-x86_64.exe", "ollydbg.exe" };
bool drm_check_process_name(const utf16** process_names, int32 count) NO_EXCEPT
{
    DWORD processes[1024], cb_needed;
    if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) {
        return false;
    }

    for (uint32 i = 0; i < cb_needed / sizeof(DWORD); ++i) {
        HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (!process_handle) {
            continue;
        }

        HMODULE mod_handle;
        DWORD cb_needed_mod;

        if (EnumProcessModules(process_handle, &mod_handle, sizeof(mod_handle), &cb_needed_mod)) {
            utf16 process_name[MAX_PATH] = L"<unknown>";

            GetModuleBaseNameW(process_handle, mod_handle, process_name, sizeof(process_name) / sizeof(char));
            for (int32 j = 0; j < count; ++j) {
                if (str_contains(process_name, process_names[j]) == 0) {
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
    WindowTitleSearchContext* ctx = (WindowTitleSearchContext *) lParam;

    wchar_t window_title[256];
    GetWindowTextW(hWnd, window_title, ARRAY_COUNT(window_title) - 1);
    window_title[255] = L'\0';

    for (int32 i = 0; i < ctx->count; ++i) {
        if (str_contains(window_title, ctx->titles[i])) {
            ctx->found |= true;
            return false;
        }
    }

    return true;
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
    const DRMSignature* __restrict patterns, size_t pattern_count,
    byte* __restrict helper_memory, size_t helper_memory_size
) NO_EXCEPT
{
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
                    const DRMSignature* sig = &patterns[p];
                    if (sig->length > bytes_read) {
                        continue;
                    }

                    // @question instead of decoding the signature we could encode the process. However, that's much slower
                    // @performance we are decrypting this over and over
                    byte decrypted_data[4096];
                    size_t decrypted_length = drm_decode(decrypted_data, sig->encrypted_pattern, sig->length);

                    if (byte_contains(helper_memory, bytes_read, decrypted_data, decrypted_length)) {
                        return true;
                    }
                }

                // The signature could be split across two chunks
                // for that reason we have to go back the maximum signature size
                // If we are already at the end of the process address space we don't move back
                if (addr + 4096 < (BYTE *) sys_info.lpMaximumApplicationAddress) {
                    addr -= 4095;
                }
            }
        }

        addr += mbi.RegionSize;
    }

    return false;
}

// Scan processes using tracking array
inline
void drm_process_scan(
    const DRMSignature* __restrict patterns, int32 pattern_count,
    DRMProcessInfo* __restrict list, int32 process_count,
    byte* __restrict helper_memory, size_t helper_memory_size
) NO_EXCEPT
{
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
            list[i].code_detected = drm_memory_scan(hProcess, patterns, pattern_count, helper_memory, helper_memory_size);

            CloseHandle(hProcess);
        }
    }
}

#endif