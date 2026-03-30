/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_H
#define COMS_STDLIB_H

/**
 * This is a collection of some basic functionality that is needed very often
 * but does by no means reflect anything remotely similar to the C/C++ standard library
 */

/**
 * Supported macro flag overview
 *
 *      Platforms:
 *          _WIN32, __linux__, PLAYSTATION_6, NINTENDO_SWITCH_2, XBOX_ONE_S
 *
 *      Bitsize:
 *          _WIN64 (= default, only when _WIN32 defined),
 *          __x86_64__ (= default),
 *          __ppc64__ (= default)
 *
 *      Architecture:
 *          __aarch64__ (= ARM), none (= x86 = default)
 *
 *      SIMD:
 *          x86
 *              __AVX512F__, __AVX2__, __SSE4_2__
 *                  on Windows you should always define all,
 *                  because it doesn't break the ABI opposite to Linux):
 *          ARM:
 *              __ARM_FEATURE_SVE, __ARM_NEON
 *
 *      Release (requires also the correct compiler arguments e.g. -O2):
 *          DEBUG_STRICT=1 (= unoptimized + heavy logging and debug/profiling info)
 *          DEBUG=1 (= unoptimized + logging/debug/profiling)
 *          INTERNAL=1 (= optimized with logging/debug/profiling info)
 *          RELEASE=1
 *
 *      Log info (automatic based on release unless you want to change it):
 *          LOG_LEVEL (default >= 1, see Log.h for default behavior)
 *
 *      Rendering API:
 *          DIRECTX_12=1, DIRECTX_11=1, OPENGL=1, VULKAN=1, SOFTWARE=1
 *
 *      Endianess:
 *          _WIN32 (= little endian), __LITTLE_ENDIAN__ (= little endian), none (= big endian)
 *
 *      C++ version:
 *          CPP_VERSION (default = 25)
 *
 *      Game stores:
 *          STEAM_STORE=1, EPIC_GAMES=1, PLAYSTATION_STORE=1, MICROSOFT_STORE=1
 *
 *      Misc.
 *          NO_STDLIB=1 (= disables stdlib usage and uses user/compiler space implementations)
 */
#include "Defines.h"
#include "Types.h"
#include "../utils/Assert.h"
#include "../compiler/CompilerUtils.h"

#include "stdlib_internal.h"
#if defined(NO_STDLIB) && NO_STDLIB
    #include "stdlib_no.h"
#else
    #include "stdlib_has.h"
#endif

#include "math_internal.h"
#if defined(NO_STDLIB) && NO_STDLIB
    #include "math_no.h"
#else
    #include "math_has.h"
#endif

#include "Helper.h"
#include "../architecture/Intrinsics.h"
#include "../platform/Platform.h"

/**
 * In general we differentiate between:
 *      Compilers (e.g. MSVC vs. GCC/Clang vs ...)
 *      Architectures (e.g. X86_64 vs. X86_32 vs. ARM)
 *      Platform (e.g. Win vs. Linux vs. Playstation vs. ...)
 *
 * Sometimes we cannot cleanly split between compiler/architecture/platform
 * and may have to also handle the compiler in the architecture files.
 *
 * Once again we are only including a very small amount of specific code here
 * and additional includes must be made when actually needed (e.g. FileUtils, SystemInfo, ...)
 */

#endif
