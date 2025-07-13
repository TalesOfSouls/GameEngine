/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_EXCEPTION_HANDLER_H
#define COMS_PLATFORM_WIN32_EXCEPTION_HANDLER_H

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../log/Log.h"

#ifdef _MSC_VER
    #pragma comment(lib, "dbghelp.lib")
#endif

void create_minidump(EXCEPTION_POINTERS *exception_pointers) {
    HANDLE fp = CreateFileA("crash_dump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fp == INVALID_HANDLE_VALUE) {
        return;
    }

    // Write the minidump
    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = exception_pointers;
    mdei.ClientPointers = FALSE;

    MiniDumpWriteDump(
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

void log_stack_trace(CONTEXT *context) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    // Initialize symbols
    SymInitialize(process, NULL, TRUE);

    // Set symbol options to load line numbers and undecorated names
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    STACKFRAME64 stack_frame = {};
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
    while (StackWalk64(machine_type, process, thread, &stack_frame, context, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL)
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

        if (SymFromAddr(process, address, NULL, symbol)) {
            LOG_1("Function: %s - Address: %l", {{LOG_DATA_CHAR_STR, symbol->Name}, {LOG_DATA_INT64, &symbol->Address}});
        } else {
            LOG_1("Function: (unknown) - Address: %l", {{LOG_DATA_INT64, &address}});
        }

        // Resolve file and line number
        IMAGEHLP_LINE64 line;
        DWORD displacement = 0;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
            LOG_1("    File: %s, Line: %l", {{LOG_DATA_CHAR_STR, line.FileName}, {LOG_DATA_INT64, &line.LineNumber}});
        } else {
            LOG_1("    File: (unknown), Line: (unknown)");
        }

        // Print module name
        IMAGEHLP_MODULE64 module_info;
        module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        if (SymGetModuleInfo64(process, address, &module_info)) {
            LOG_1("    Module: %s", {{LOG_DATA_CHAR_STR, module_info.ModuleName}});
        } else {
            LOG_1("    Module: (unknown)");
        }
    }

    LOG_TO_FILE();
    SymCleanup(process);
}

void print_stack_trace(CONTEXT *context) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    // Initialize symbols
    SymInitialize(process, NULL, TRUE);

    STACKFRAME64 stack_frame = {};
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;

    // Initialize stack frame for x64
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    printf("Stack trace:\n");

    while (StackWalk64(machine_type, process, thread, &stack_frame, context, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL)
    ) {
        DWORD64 address = stack_frame.AddrPC.Offset;

        // Resolve symbol information
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (SymFromAddr(process, address, NULL, symbol)) {
            printf("Function: %s - Address: 0x%llx\n", symbol->Name, symbol->Address);
        } else {
            printf("Function: (unknown) - Address: 0x%llx\n", address);
        }

        // Resolve file and line number
        IMAGEHLP_LINE64 line;
        DWORD displacement = 0;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
            printf("    File: %s, Line: %lu\n", line.FileName, line.LineNumber);
        } else {
            printf("    File: (unknown), Line: (unknown)\n");
        }
    }

    SymCleanup(process);
}

#endif