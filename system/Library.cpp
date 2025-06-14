/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_LIBRARY_C
#define COMS_SYSTEM_LIBRARY_C

#if _WIN32
    #include "../platform/win32/Library.cpp"
#elif __linux__
    #include "../platform/linux/Library.cpp"
#endif

#endif