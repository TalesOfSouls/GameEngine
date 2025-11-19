#ifndef COMS_PLATFORM_WIN32_DRM
#define COMS_PLATFORM_WIN32_DRM

#include <windows.h>
#include <psapi.h>
#include <winternl.h>
#include "../../stdlib/Types.h"

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
bool drm_prevent_debugger_attach()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
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

bool drm_is_being_debugged()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        return false;
    }

    pNtQueryInformationProcess NtQueryInformationProcess = (pNtQueryInformationProcess)
        GetProcAddress(ntdll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) return false;

    PROCESS_BASIC_INFORMATION pbi = {};
    ULONG returnLength = 0;

    NTSTATUS status = NtQueryInformationProcess(
        GetCurrentProcess(),
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),
        &returnLength
    );

    if (status != 0) return false;

    // Read BeingDebugged flag from PEB
    BYTE* pPeb = (BYTE*)pbi.PebBaseAddress;

    return pPeb[2] != 0;
}

#include <bcrypt.h>
#include <string.h>
#pragma comment(lib, "bcrypt.lib")

bool drm_verify_code_integrity(
    byte* __restrict exe_buffer, uint32 buffer_length,
    const byte* __restrict expected_hash, uint32 expected_hash_len
) {
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    BYTE* hash_object = NULL;
    BYTE calculated_hash[20]; // SHA-1 = 20 bytes
    DWORD hash_object_size = 0;
    DWORD result_size = 0;

    NTSTATUS status;

    // We are not loading the exe, we are loading the exe as it is currently in memory
    char module_path[MAX_PATH];
    GetModuleFileNameA(NULL, module_path, MAX_PATH);

    HANDLE in_memory_handle = CreateFileA(module_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (in_memory_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD file_size = GetFileSize(in_memory_handle, NULL);
    if (buffer_length < file_size) {
        LOG_1("Buffer can't hold size");

        return false;
    }

    DWORD bytes_read = 0;
    ReadFile(in_memory_handle, exe_buffer, file_size, &bytes_read, NULL);
    CloseHandle(in_memory_handle);

    if (bytes_read <= 0) {
        return false;
    }

    // Open SHA-1 algorithm provider
    status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) {
        return false;
    }

    // Get the size of the hash object
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hash_object_size, sizeof(DWORD), &result_size, 0);
    if (!BCRYPT_SUCCESS(status)) {
        goto cleanup;
    }

    hash_object = (BYTE*) HeapAlloc(GetProcessHeap(), 0, hash_object_size);
    if (!hash_object) {
        goto cleanup;
    }

    // Create the hash object
    status = BCryptCreateHash(algorithm, &hHash, hash_object, hash_object_size, NULL, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        goto cleanup;
    }

    // Hash the input buffer
    status = BCryptHashData(hHash, (PUCHAR)exe_buffer, (ULONG)bytes_read, 0);
    if (!BCRYPT_SUCCESS(status)) {
        goto cleanup;
    }

    // Finalize and retrieve the hash
    status = BCryptFinishHash(hHash, calculated_hash, sizeof(calculated_hash), 0);
    if (!BCRYPT_SUCCESS(status)) {
        goto cleanup;
    }

    // Compare the calculated hash with the expected one
    if (expected_hash_len != sizeof(calculated_hash)) {
        goto cleanup;
    }
    if (memcmp(calculated_hash, expected_hash, expected_hash_len) != 0) {
        goto cleanup;
    }

    // Success
    HeapFree(GetProcessHeap(), 0, hash_object);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(algorithm, 0);

    return true;

    cleanup:
        if (hash_object) {
            HeapFree(GetProcessHeap(), 0, hash_object);
        }

        if (hHash) {
            BCryptDestroyHash(hHash);
        }

        if (algorithm) {
            BCryptCloseAlgorithmProvider(algorithm, 0);
        }

    return false;
}


// Example: process_names = { "x64dbg.exe", "cheatengine-x86_64.exe", "ollydbg.exe" };
bool drm_check_process_name(const char** process_names, int32 count) {
    DWORD processes[1024], cb_needed;
    if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) {
        return false;
    }

    for (uint32 i = 0; i < cb_needed / sizeof(DWORD); ++i) {
        HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (!process_handle) {
            continue;
        }

        char process_name[MAX_PATH] = "<unknown>";

        HMODULE mod_handle;
        DWORD cb_needed_mod;

        if (EnumProcessModules(process_handle, &mod_handle, sizeof(mod_handle), &cb_needed_mod)) {
            GetModuleBaseNameA(process_handle, mod_handle, process_name, sizeof(process_name) / sizeof(char));
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
    const char** titles;
    int32 count;
    bool found;
};

static BOOL CALLBACK drm_enum_windows_callback(HWND hWnd, LPARAM lParam) {
    WindowTitleSearchContext* ctx = (WindowTitleSearchContext *) lParam;

    char window_title[256];
    GetWindowTextA(hWnd, window_title, sizeof(window_title) - 1);
    window_title[255] = '\0';

    for (int32 i = 0; i < ctx->count; ++i) {
        if (str_contains(window_title, ctx->titles[i])) {
            ctx->found = true;
            return false;
        }
    }

    return true;
}

inline
bool drm_check_window_title(const char** window_titles, int32 count) {
    WindowTitleSearchContext ctx = { window_titles, count, false };
    EnumWindows(drm_enum_windows_callback, (LPARAM) &ctx);

    return ctx.found;
}

#endif