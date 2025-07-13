/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_CPU_INFO_H
#define COMS_ARCHITECTURE_CPU_INFO_H

#include "../stdlib/Types.h"

// @todo We don't care about a lot of features, consider to remove them if we can get either below 32bit
// or if we eventually overflow 64 bit
enum CPUFeature : uint64 {
    CPU_FEATURE_SSE3 = 1ULL << 0,
    CPU_FEATURE_PCLMULQDQ = 1ULL << 1,
    CPU_FEATURE_MONITOR = 1ULL << 2,
    CPU_FEATURE_SSSE3 = 1ULL << 3,
    CPU_FEATURE_FMA = 1ULL << 4,
    CPU_FEATURE_CMPXCHG16B = 1ULL << 5,
    CPU_FEATURE_SSE41 = 1ULL << 6,
    CPU_FEATURE_SSE42 = 1ULL << 7,
    CPU_FEATURE_MOVBE = 1ULL << 8,
    CPU_FEATURE_POPCNT = 1ULL << 9,
    CPU_FEATURE_AES = 1ULL << 10,
    CPU_FEATURE_XSAVE = 1ULL << 11,
    CPU_FEATURE_OSXSAVE = 1ULL << 12,
    CPU_FEATURE_AVX = 1ULL << 13,
    CPU_FEATURE_F16C = 1ULL << 14,
    CPU_FEATURE_RDRAND = 1ULL << 15,
    CPU_FEATURE_MSR = 1ULL << 16,
    CPU_FEATURE_CX8 = 1ULL << 17,
    CPU_FEATURE_SEP = 1ULL << 18,
    CPU_FEATURE_CMOV = 1ULL << 19,
    CPU_FEATURE_CLFSH = 1ULL << 20,
    CPU_FEATURE_MMX = 1ULL << 21,
    CPU_FEATURE_FXSR = 1ULL << 22,
    CPU_FEATURE_SSE = 1ULL << 23,
    CPU_FEATURE_SSE2 = 1ULL << 24,
    CPU_FEATURE_AVX2 = 1ULL << 25,
    CPU_FEATURE_BMI1 = 1ULL << 26,
    CPU_FEATURE_BMI2 = 1ULL << 27,
    CPU_FEATURE_ERMS = 1ULL << 28,
    CPU_FEATURE_INVPCID = 1ULL << 29,
    CPU_FEATURE_RDSEED = 1ULL << 30,
    CPU_FEATURE_ADX = 1ULL << 31,
    CPU_FEATURE_SHA = 1ULL << 32,
    CPU_FEATURE_PREFETCHWT1 = 1ULL << 33,
    CPU_FEATURE_LAHF = 1ULL << 34,
    CPU_FEATURE_LZCNT = 1ULL << 35,
    CPU_FEATURE_ABM = 1ULL << 36,
    CPU_FEATURE_SSE4a = 1ULL << 37,
    CPU_FEATURE_XOP = 1ULL << 38,
    CPU_FEATURE_TBM = 1ULL << 39,
    CPU_FEATURE_SYSCALL = 1ULL << 40,
    CPU_FEATURE_MMEXT = 1ULL << 41,
    CPU_FEATURE_RDTSCP = 1ULL << 42,
    CPU_FEATURE_3DNOWEXT = 1ULL << 43,
    CPU_FEATURE_3DNOW = 1ULL << 44,
    CPU_FEATURE_AVX512F = 1ULL << 45,
    CPU_FEATURE_AVX512PF = 1ULL << 46,
    CPU_FEATURE_AVX512ER = 1ULL << 47,
    CPU_FEATURE_AVX512CD = 1ULL << 48,

    CPU_FEATURE_ASIMD_ARM = 1ULL << 49,
    CPU_FEATURE_ATOMICS_ARM = 1ULL << 50,
    CPU_FEATURE_PMULL_ARM = 1ULL << 51,
    CPU_FEATURE_SVEPMULL_ARM = 1ULL << 52,
    CPU_FEATURE_NEON_ARM = 1ULL << 53,
    CPU_FEATURE_SVE_ARM = 1ULL << 54,
    CPU_FEATURE_SVE2_ARM = 1ULL << 55,
    CPU_FEATURE_AES_ARM = 1ULL << 56,
    CPU_FEATURE_SHA1_ARM = 1ULL << 57,
    CPU_FEATURE_SHA2_ARM = 1ULL << 58,
    CPU_FEATURE_CRC32_ARM = 1ULL << 59,
};

struct CpuCacheInfo {
    uint32 size;
    uint32 sets;
    byte level;
    byte ways;
    byte partitions;
    uint16 line_size;
};

struct CpuInfo {
    char vendor[13];
    char brand[49];
    byte model;
    byte family;
    int16 core_count;
    int32 mhz;
    uint32 page_size;
    CpuCacheInfo cache[4];
    uint64 features;
};

#endif