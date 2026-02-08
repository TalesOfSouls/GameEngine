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

/**
 * We support 2 ways how to load libraries:
 *      1. Simply use the very low level wrapper functions for library loading (library_dyn_*)
 *      2. Use the slightly more abstracted version were you can pass in a struct that contains all the information
 *          The benefit of this is that you just modify and manage 2 structs and the loader handles the initialization for all elements
 */

#if _WIN32
    #include "../platform/win32/Library.cpp"
#elif __linux__
    #include "../platform/linux/Library.cpp"
#endif

#endif