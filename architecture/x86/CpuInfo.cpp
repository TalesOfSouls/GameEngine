/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_X86_CPU_INFO_C
#define COMS_ARCHITECTURE_X86_CPU_INFO_C

#include <stdio.h>
#include <stdint.h>
#include "../../stdlib/Types.h"
#include "../CpuInfo.h"
#include "../../compiler/CompilerUtils.h"

#define svcntw() 0

uint64 cpu_info_features() {
    uint64 feature_bitfield = 0;
    uint32 cpu_info[4] = {0};

    compiler_cpuid(cpu_info, 0x00000001);
    uint32 ecx = (uint32) cpu_info[2];
    uint32 edx = (uint32) cpu_info[3];

    // Map ECX features
    if (ecx & (1 << 0)) feature_bitfield |= CPU_FEATURE_SSE3;
    if (ecx & (1 << 1)) feature_bitfield |= CPU_FEATURE_PCLMULQDQ;
    if (ecx & (1 << 3)) feature_bitfield |= CPU_FEATURE_MONITOR;
    if (ecx & (1 << 9)) feature_bitfield |= CPU_FEATURE_SSSE3;
    if (ecx & (1 << 12)) feature_bitfield |= CPU_FEATURE_FMA;
    if (ecx & (1 << 13)) feature_bitfield |= CPU_FEATURE_CMPXCHG16B;
    if (ecx & (1 << 19)) feature_bitfield |= CPU_FEATURE_SSE41;
    if (ecx & (1 << 20)) feature_bitfield |= CPU_FEATURE_SSE42;
    if (ecx & (1 << 22)) feature_bitfield |= CPU_FEATURE_MOVBE;
    if (ecx & (1 << 23)) feature_bitfield |= CPU_FEATURE_POPCNT;
    if (ecx & (1 << 25)) feature_bitfield |= CPU_FEATURE_AES;
    if (ecx & (1 << 26)) feature_bitfield |= CPU_FEATURE_XSAVE;
    if (ecx & (1 << 27)) feature_bitfield |= CPU_FEATURE_OSXSAVE;
    if (ecx & (1 << 28)) feature_bitfield |= CPU_FEATURE_AVX;
    if (ecx & (1 << 29)) feature_bitfield |= CPU_FEATURE_F16C;
    if (ecx & (1 << 30)) feature_bitfield |= CPU_FEATURE_RDRAND;

    // Map EDX features
    if (edx & (1 << 5)) feature_bitfield |= CPU_FEATURE_MSR;
    if (edx & (1 << 8)) feature_bitfield |= CPU_FEATURE_CX8;
    if (edx & (1 << 11)) feature_bitfield |= CPU_FEATURE_SEP;
    if (edx & (1 << 15)) feature_bitfield |= CPU_FEATURE_CMOV;
    if (edx & (1 << 19)) feature_bitfield |= CPU_FEATURE_CLFSH;
    if (edx & (1 << 23)) feature_bitfield |= CPU_FEATURE_MMX;
    if (edx & (1 << 24)) feature_bitfield |= CPU_FEATURE_FXSR;
    if (edx & (1 << 25)) feature_bitfield |= CPU_FEATURE_SSE;
    if (edx & (1 << 26)) feature_bitfield |= CPU_FEATURE_SSE2;

    compiler_cpuid(cpu_info, 0x00000007);
    uint32 ebx = (uint32) cpu_info[1];
    uint32 ecx7 = (uint32) cpu_info[2];

    // Map EBX features
    if (ebx & (1 << 3)) feature_bitfield |= CPU_FEATURE_BMI1;
    if (ebx & (1 << 5)) feature_bitfield |= CPU_FEATURE_AVX2;
    if (ebx & (1 << 8)) feature_bitfield |= CPU_FEATURE_BMI2;
    if (ebx & (1 << 9)) feature_bitfield |= CPU_FEATURE_ERMS;
    if (ebx & (1 << 10)) feature_bitfield |= CPU_FEATURE_INVPCID;
    if (ebx & (1 << 16)) feature_bitfield |= CPU_FEATURE_AVX512F;
    if (ebx & (1 << 18)) feature_bitfield |= CPU_FEATURE_RDSEED;
    if (ebx & (1 << 19)) feature_bitfield |= CPU_FEATURE_ADX;
    if (ebx & (1 << 26)) feature_bitfield |= CPU_FEATURE_AVX512PF;
    if (ebx & (1 << 27)) feature_bitfield |= CPU_FEATURE_AVX512ER;
    if (ebx & (1 << 28)) feature_bitfield |= CPU_FEATURE_AVX512CD;
    if (ebx & (1 << 29)) feature_bitfield |= CPU_FEATURE_SHA;


    // Map ECX features
    if (ecx7 & (1 << 0)) feature_bitfield |= CPU_FEATURE_PREFETCHWT1;

    compiler_cpuid(cpu_info, 0x80000001);
    uint32 ecx81 = (uint32) cpu_info[2];
    uint32 edx81 = (uint32) cpu_info[3];

    // Map ECX extended features
    if (ecx81 & (1 << 0)) feature_bitfield |= CPU_FEATURE_LAHF;
    if (ecx81 & (1 << 5)) feature_bitfield |= CPU_FEATURE_LZCNT;
    if (ecx81 & (1 << 6)) feature_bitfield |= CPU_FEATURE_ABM;
    if (ecx81 & (1 << 11)) feature_bitfield |= CPU_FEATURE_XOP;
    if (ecx81 & (1 << 21)) feature_bitfield |= CPU_FEATURE_TBM;

    // Map EDX extended features
    if (edx81 & (1 << 11)) feature_bitfield |= CPU_FEATURE_SYSCALL;
    if (edx81 & (1 << 22)) feature_bitfield |= CPU_FEATURE_MMEXT;
    if (edx81 & (1 << 27)) feature_bitfield |= CPU_FEATURE_RDTSCP;
    if (edx81 & (1 << 30)) feature_bitfield |= CPU_FEATURE_3DNOWEXT;
    if (edx81 & (1 << 31)) feature_bitfield |= CPU_FEATURE_3DNOW;

    return feature_bitfield;
}

void cpu_info_cache(byte level, CpuCacheInfo* cache) {
    uint32 eax, ebx, ecx; //, edx;
    int32 type;

    cache->level = level;
    cache->size = 0;
    cache->ways = 0;
    cache->partitions = 0;
    cache->sets = 0;
    cache->line_size = 0;

    uint32 regs[4];
    compiler_cpuid(regs, 0x04, level);
    eax = regs[0];
    ebx = regs[1];
    ecx = regs[2];
    // edx = regs[3];

    type = (eax & 0x1F);

    if (type == 0) {
        return;
    }

    cache->ways = (byte) ((ebx >> 22) & 0x3FF) + 1;
    cache->partitions = (byte) ((ebx >> 12) & 0x3FF) + 1;
    cache->line_size = OMS_MAX((uint16) (ebx & 0xFFF) + 1, 64);
    cache->sets = ecx + 1;
    cache->size = cache->ways * cache->partitions * cache->line_size * cache->sets;
}

#endif