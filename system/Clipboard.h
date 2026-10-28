/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_CLIPBOARD_H
#define COMS_SYSTEM_CLIPBOARD_H

#if _WIN32
    #include "../platform/win32/Clipboard.h"
#elif __linux__
    #include "../platform/linux/Clipboard.h"
#endif

#endif
