/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_INTRINSICS_H
#define COMS_ARCHITECTURE_INTRINSICS_H

#ifdef __aarch64__
    #include "arm/Intrinsics.h"
#else
    #include "x86/Intrinsics.h"
#endif

#endif