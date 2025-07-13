/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_ARM_CPU_INFO_C
#define COMS_ARCHITECTURE_ARM_CPU_INFO_C

#include <stdio.h>
#include <stdint.h>
#include "../../stdlib/Types.h"
#include "../CpuInfo.h"
#include <arm_sve.h>

uint64 cpu_info_features() {
    uint64 feature_bitfield = 0;

    uint64 hwcap = getauxval(AT_HWCAP);
    uint64 hwcap2 = getauxval(AT_HWCAP2);

    if (hwcap & HWCAP_ASIMD) feature_bitfield |= CPU_FEATURE_ASIMD_ARM;
    if (hwcap & HWCAP_ATOMICS) feature_bitfield |= CPU_FEATURE_ATOMICS_ARM;
    if (hwcap & HWCAP_PMULL) feature_bitfield |= CPU_FEATURE_PMULL_ARM;
    if (hwcap2 & HWCAP2_SVEPMULL) feature_bitfield |= CPU_FEATURE_SVEPMULL_ARM;
    if (hwcap & HWCAP_NEON) feature_bitfield |= CPU_FEATURE_NEON_ARM;
    if (hwcap & HWCAP_SVE) feature_bitfield |= CPU_FEATURE_SVE_ARM;
    if (hwcap2 & HWCAP2_SVE) feature_bitfield |= CPU_FEATURE_SVE2_ARM;
    if (hwcap & HWCAP_AES) feature_bitfield |= CPU_FEATURE_AES_ARM;
    if (hwcap & HWCAP_SHA1) feature_bitfield |= CPU_FEATURE_SHA1_ARM;
    if (hwcap & HWCAP_SHA2) feature_bitfield |= CPU_FEATURE_SHA2_ARM;
    if (hwcap & HWCAP_CRC32) feature_bitfield |= CPU_FEATURE_CRC32_ARM;

    return feature_bitfield;
}

void cpu_info_cache(int32 level, CpuCacheInfo* cache) {
    uint32 ccsidr; // Cache Size ID Register
    uint32 line_size, associativity, num_sets;

    cache->level = level;
    cache->size = 0;
    cache->ways = 0;
    cache->partitions = 1; // Typically 1 on ARM
    cache->sets = 0;
    cache->line_size = 0;

    // Select cache level and type in CSSELR
    uint32 csselr = level << 1; // Data or unified cache
    asm volatile("msr csselr_el1, %0" : : "r" (csselr));
    asm volatile("mrs %0, ccsidr_el1" : "=r" (ccsidr));

    // Extract cache details from CCSIDR
    line_size = (ccsidr & 0x7) + 4; // Line size in log2 words
    associativity = ((ccsidr >> 3) & 0x3FF) + 1;
    num_sets = ((ccsidr >> 13) & 0x7FFF) + 1;

    // Populate the cache structure
    cache->line_size = 1 << line_size; // Convert to bytes
    cache->ways = associativity;
    cache->sets = num_sets;
    cache->size = cache->ways * cache->sets * cache->line_size;
}

#endif