/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_FAST_PIPES_H
#define COMS_PLATFORM_WIN32_FAST_PIPES_H

#include "../stdlib/Types.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

static int ENABLE_FAST_PIPES()
{
    int32 result = 0;

    wchar_t pipe_name[32];
    wsprintfW(pipe_name, L"\\\\.\\pipe\\fastpipe%x", GetCurrentProcessId());
    HANDLE fast_pip = CreateFileW(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if(fast_pip != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, fast_pip);
        SetStdHandle(STD_INPUT_HANDLE, fast_pip);

        int32 std_out = _open_osfhandle((intptr_t) fast_pip, O_WRONLY | O_TEXT);
        int32 std_in = _open_osfhandle((intptr_t) fast_pip, O_RDONLY | O_TEXT);

        _dup2(std_out, _fileno(stdout));
        _dup2(std_in, _fileno(stdin));

        _close(std_out);
        _close(std_in);

        result = 1;
    }

    return result;
}

#endif