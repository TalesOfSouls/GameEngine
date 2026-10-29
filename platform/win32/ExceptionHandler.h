/**
 * Exception handling
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_EXCEPTION_HANDLER_H
#define COMS_PLATFORM_WIN32_EXCEPTION_HANDLER_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../log/Log.h"

#include "libs/dbghelp.h"

// The dbghelp functions are loaded at runtime (dbghelp_init), they are NULL if the library couldn't be loaded
#if defined(COMS_IMPLICIT_LINKING) && COMS_IMPLICIT_LINKING
    #define DBGHELP_LOADED(f) true
#else
    #define DBGHELP_LOADED(f) ((f) != NULL)
#endif

void create_minidump(EXCEPTION_POINTERS* exception_pointers) NO_EXCEPT
{
    if (!DBGHELP_LOADED(DBGHELP_MiniDumpWriteDump)) {
        return;
    }

    HANDLE fp = CreateFileW(L"crash_dump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fp == INVALID_HANDLE_VALUE) {
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = exception_pointers;
    mdei.ClientPointers = FALSE;

    DBGHELP_MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        fp,
        (MINIDUMP_TYPE) (MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithModuleHeaders |
            MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
            MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithTokenInformation),
        (exception_pointers != NULL) ? &mdei : NULL,
        NULL,
        NULL
    );

    CloseHandle(fp);
}

void log_stack_trace(CONTEXT* context) NO_EXCEPT
{
    if (!DBGHELP_LOADED(DBGHELP_StackWalk64) || !DBGHELP_LOADED(DBGHELP_SymInitialize)) {
        LOG_1("Stack trace: dbghelp.dll not available");
        return;
    }

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    DBGHELP_SymInitialize(process, NULL, TRUE);

    // Set symbol options to load line numbers and undecorated names
    DBGHELP_SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    STACKFRAME64 stack_frame = {0};
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;

    // Initialize stack frame for x64
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    LOG_1("Stack trace:");

    // Walk the stack
    while (DBGHELP_StackWalk64(machine_type, process, thread, &stack_frame, context, NULL,
        DBGHELP_SymFunctionTableAccess64, DBGHELP_SymGetModuleBase64, NULL)
    ) {
        DWORD64 address = stack_frame.AddrPC.Offset;

        // Skip invalid addresses
        if (address == 0) {
            break;
        }

        // Resolve symbol information
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (DBGHELP_SymFromAddr(process, address, NULL, symbol)) {
            LOG_1(
                "Function: %s - Address: %l",
                {DATA_TYPE_CHAR_STR, symbol->Name},
                {DATA_TYPE_INT64, &symbol->Address}
            );
        } else {
            LOG_1("Function: (unknown) - Address: %l", {DATA_TYPE_INT64, &address});
        }

        // Resolve file and line number
        IMAGEHLP_LINE64 line;
        DWORD displacement = 0;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (DBGHELP_SymGetLineFromAddr64(process, address, &displacement, &line)) {
            LOG_1(
                "    File: %s, Line: %l",
                {DATA_TYPE_CHAR_STR, line.FileName},
                {DATA_TYPE_INT64, &line.LineNumber}
            );
        } else {
            LOG_1("    File: (unknown), Line: (unknown)");
        }

        // Print module name
        IMAGEHLP_MODULE64 module_info;
        module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        if (DBGHELP_SymGetModuleInfo64(process, address, &module_info)) {
            LOG_1("    Module: %s", {DATA_TYPE_CHAR_STR, module_info.ModuleName});
        } else {
            LOG_1("    Module: (unknown)");
        }
    }

    LOG_TO_FILE();
    DBGHELP_SymCleanup(process);
}

void print_stack_trace(CONTEXT* context) NO_EXCEPT
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    DBGHELP_SymInitialize(process, NULL, TRUE);

    STACKFRAME64 stack_frame = {0};
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;

    // Initialize stack frame for x64
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    LOG_1("Stack trace:");

    while (DBGHELP_StackWalk64(machine_type, process, thread, &stack_frame, context, NULL,
        DBGHELP_SymFunctionTableAccess64, DBGHELP_SymGetModuleBase64, NULL)
    ) {
        DWORD64 address = stack_frame.AddrPC.Offset;

        // Resolve symbol information
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (DBGHELP_SymFromAddr(process, address, NULL, symbol)) {
            LOG_1(
                "Function: %s - Address: 0x%l\n",
                {DATA_TYPE_CHAR_STR, symbol->Name},
                {DATA_TYPE_INT64, &symbol->Address}
            );
        } else {
            LOG_1("Function: (unknown) - Address: 0x%l\n", {DATA_TYPE_INT64, &address});
        }

        // Resolve file and line number
        IMAGEHLP_LINE64 line;
        DWORD displacement = 0;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (DBGHELP_SymGetLineFromAddr64(process, address, &displacement, &line)) {
            LOG_1(
                "    File: %s, Line: %l",
                {DATA_TYPE_CHAR_STR, line.FileName},
                {DATA_TYPE_INT64, &line.LineNumber}
            );
        } else {
            LOG_1("    File: (unknown), Line: (unknown)\n");
        }
    }

    LOG_TO_FILE();
    DBGHELP_SymCleanup(process);
}

/**
 * Crash + hang reports that work in a broken process state
 *
 * WARNING: The functions below don't use the log system (its lock may be held by the crashed/hung thread)
 *          They write directly to their own files next to the executable.
 */

// Absolute path of a file next to the executable
static inline
void exception_file_path(wchar_t* const __restrict path, int32 capacity, const wchar_t* const __restrict file) NO_EXCEPT
{
    const DWORD length = GetModuleFileNameW(NULL, path, (DWORD) capacity);

    int32 dir_end = 0;
    for (int32 i = 0; i < (int32) length; ++i) {
        if (path[i] == L'\\' || path[i] == L'/') {
            dir_end = i + 1;
        }
    }

    int32 i = 0;
    while (file[i] && dir_end + i < capacity - 1) {
        path[dir_end + i] = file[i];
        ++i;
    }

    path[dir_end + i] = L'\0';
}

static inline
void exception_file_write(HANDLE file, const char* const text) NO_EXCEPT
{
    DWORD written;
    WriteFile(file, text, (DWORD) strlen(text), &written, NULL);
}

static inline
bool exception_dbghelp_available() NO_EXCEPT
{
    return DBGHELP_LOADED(DBGHELP_StackWalk64) && DBGHELP_LOADED(DBGHELP_SymInitialize) && DBGHELP_LOADED(DBGHELP_SymFromAddr)
        && DBGHELP_LOADED(DBGHELP_SymFunctionTableAccess64) && DBGHELP_LOADED(DBGHELP_SymGetModuleBase64);
}

/**
 * Collects the return addresses of a thread
 *
 * @param thread    Thread handle (must be suspended if it isn't the current thread)
 * @param context   Register state of the thread, it is modified
 *
 * @return Number of addresses
 */
static inline
int32 exception_stack_collect(HANDLE thread, CONTEXT* const context, DWORD64* const addresses, int32 capacity) NO_EXCEPT
{
    if (!exception_dbghelp_available()) {
        return 0;
    }

    STACKFRAME64 frame = {};
    #if defined(_M_ARM64)
        const DWORD machine_type = IMAGE_FILE_MACHINE_ARM64;
        frame.AddrPC.Offset = context->Pc;
        frame.AddrFrame.Offset = context->Fp;
        frame.AddrStack.Offset = context->Sp;
    #else
        const DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context->Rip;
        frame.AddrFrame.Offset = context->Rbp;
        frame.AddrStack.Offset = context->Rsp;
    #endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    int32 count = 0;
    while (count < capacity
        && DBGHELP_StackWalk64(
            machine_type, GetCurrentProcess(), thread, &frame, context, NULL,
            DBGHELP_SymFunctionTableAccess64, DBGHELP_SymGetModuleBase64, NULL
        )
    ) {
        if (!frame.AddrPC.Offset) {
            break;
        }

        addresses[count++] = frame.AddrPC.Offset;
    }

    return count;
}

// Writes the addresses as "module!function + offset  file:line" (SymInitialize must be called before)
static inline
void exception_stack_write(HANDLE file, const DWORD64* const addresses, int32 count) NO_EXCEPT
{
    if (!count) {
        exception_file_write(file, "    (no stack trace, dbghelp.dll not available)\r\n");
        return;
    }

    const HANDLE process = GetCurrentProcess();

    for (int32 i = 0; i < count; ++i) {
        const DWORD64 address = addresses[i];

        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 offset = 0;
        const char* name = "(unknown)";
        if (DBGHELP_SymFromAddr(process, address, &offset, symbol)) {
            name = symbol->Name;
        }

        IMAGEHLP_MODULE64 module_info = {};
        module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        const char* module = "?";
        if (DBGHELP_LOADED(DBGHELP_SymGetModuleInfo64) && DBGHELP_SymGetModuleInfo64(process, address, &module_info)) {
            module = module_info.ModuleName;
        }

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement = 0;

        char text[1024];
        if (DBGHELP_LOADED(DBGHELP_SymGetLineFromAddr64) && DBGHELP_SymGetLineFromAddr64(process, address, &displacement, &line)) {
            snprintf(
                text, sizeof(text), "    %2d %s!%s + 0x%llx    %s:%lu\r\n",
                i, module, name, (unsigned long long) offset, line.FileName, (unsigned long) line.LineNumber
            );
        } else {
            snprintf(
                text, sizeof(text), "    %2d %s!%s + 0x%llx    (0x%llx)\r\n",
                i, module, name, (unsigned long long) offset, (unsigned long long) address
            );
        }

        exception_file_write(file, text);
    }
}

// Minidump of the whole process (all threads), exception_pointers may be NULL (e.g. hang)
static inline
void exception_minidump_write(const wchar_t* const path, EXCEPTION_POINTERS* const exception_pointers, DWORD thread_id) NO_EXCEPT
{
    if (!DBGHELP_LOADED(DBGHELP_MiniDumpWriteDump)) {
        return;
    }

    HANDLE fp = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fp == INVALID_HANDLE_VALUE) {
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = thread_id;
    mdei.ExceptionPointers = exception_pointers;
    mdei.ClientPointers = FALSE;

    DBGHELP_MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        fp,
        (MINIDUMP_TYPE) (MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithUnloadedModules
            | MiniDumpWithProcessThreadData | MiniDumpWithThreadInfo),
        exception_pointers ? &mdei : NULL,
        NULL,
        NULL
    );

    CloseHandle(fp);
}

#endif