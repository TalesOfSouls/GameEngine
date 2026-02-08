/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_LIB_OLE32_STATIC_H
#define COMS_PLATFORM_WIN32_LIB_OLE32_STATIC_H

#include "../../../stdlib/Stdlib.h"
#include <windows.h>
#include <objbase.h>
#include <combaseapi.h>

#pragma comment(lib, "ole32.lib")

bool ole32_static_init() NO_EXCEPT
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

#endif