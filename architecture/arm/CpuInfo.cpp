/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ARCHITECTURE_ARM_CPU_INFO_C
#define COMS_ARCHITECTURE_ARM_CPU_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../CpuInfo.h"

// ARM has no cpuid + the cache registers (ccsidr_el1, csselr_el1) are not readable from user mode
// -> the operating system provides the cpu features + cache information
//      Linux:   getauxval (features), /sys/devices/system/cpu/cpu0/cache (caches)
//      Windows: IsProcessorFeaturePresent (features), GetLogicalProcessorInformation (caches)
#if _WIN32
    #include <windows.h>

    uint64 cpu_info_features() NO_EXCEPT
    {
        uint64 feature_bitfield = 0;

        // NEON (= ASIMD) is part of every AArch64 cpu
        feature_bitfield |= CPU_FEATURE_ASIMD_ARM | CPU_FEATURE_NEON_ARM;

        #ifdef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
            if (IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE)) {
                feature_bitfield |= CPU_FEATURE_AES_ARM | CPU_FEATURE_PMULL_ARM | CPU_FEATURE_SHA1_ARM | CPU_FEATURE_SHA2_ARM;
            }
        #endif

        #ifdef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
            if (IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE)) {
                feature_bitfield |= CPU_FEATURE_CRC32_ARM;
            }
        #endif

        #ifdef PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
            if (IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE)) {
                feature_bitfield |= CPU_FEATURE_ATOMICS_ARM;
            }
        #endif

        // Only in newer Windows SDKs
        #ifdef PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
            if (IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)) {
                feature_bitfield |= CPU_FEATURE_SVE_ARM;
            }
        #endif

        #ifdef PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE
            if (IsProcessorFeaturePresent(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE)) {
                feature_bitfield |= CPU_FEATURE_SVE2_ARM;
            }
        #endif

        return feature_bitfield;
    }

    /**
     * @param byte level Cache index (same as the x86 version): 0 = L1 data, 1 = L1 instruction, 2 = L2, 3 = L3
     */
    void cpu_info_cache(byte level, CpuCacheInfo* cache) NO_EXCEPT
    {
        cache->level = level;
        cache->size = 0;
        cache->ways = 0;
        cache->partitions = 1;
        cache->sets = 0;
        cache->line_size = 0;

        const BYTE cache_level = level <= 1 ? 1 : (BYTE) level;
        const PROCESSOR_CACHE_TYPE cache_type = level == 0
            ? CacheData
            : (level == 1 ? CacheInstruction : CacheUnified);

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer[256];
        DWORD length = sizeof(buffer);
        if (!GetLogicalProcessorInformation(buffer, &length)) {
            return;
        }

        const DWORD count = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        for (DWORD i = 0; i < count; ++i) {
            if (buffer[i].Relationship != RelationCache) {
                continue;
            }

            const CACHE_DESCRIPTOR* const desc = &buffer[i].Cache;
            if (desc->Level != cache_level
                || (desc->Type != cache_type && !(cache_type == CacheUnified && desc->Type == CacheData))
            ) {
                continue;
            }

            cache->size = desc->Size;
            cache->ways = desc->Associativity;
            cache->line_size = desc->LineSize;
            cache->sets = desc->LineSize && desc->Associativity
                ? desc->Size / (desc->LineSize * desc->Associativity)
                : 0;

            return;
        }
    }
#else
    #include <sys/auxv.h>
    #include <asm/hwcap.h>

    // SystemInfo.cpp includes this file before FileUtils.cpp
    #include "../../utils/StringUtils.h"
    #include "../../system/FileUtils.cpp"

    uint64 cpu_info_features() NO_EXCEPT
    {
        uint64 feature_bitfield = 0;

        const uint64 hwcap = getauxval(AT_HWCAP);
        const uint64 hwcap2 = getauxval(AT_HWCAP2);

        // On AArch64 NEON = ASIMD (HWCAP_NEON only exists on 32 bit ARM)
        if (hwcap & HWCAP_ASIMD) feature_bitfield |= CPU_FEATURE_ASIMD_ARM | CPU_FEATURE_NEON_ARM;
        if (hwcap & HWCAP_ATOMICS) feature_bitfield |= CPU_FEATURE_ATOMICS_ARM;
        if (hwcap & HWCAP_PMULL) feature_bitfield |= CPU_FEATURE_PMULL_ARM;
        if (hwcap & HWCAP_AES) feature_bitfield |= CPU_FEATURE_AES_ARM;
        if (hwcap & HWCAP_SHA1) feature_bitfield |= CPU_FEATURE_SHA1_ARM;
        if (hwcap & HWCAP_SHA2) feature_bitfield |= CPU_FEATURE_SHA2_ARM;
        if (hwcap & HWCAP_CRC32) feature_bitfield |= CPU_FEATURE_CRC32_ARM;

        #ifdef HWCAP_SVE
            if (hwcap & HWCAP_SVE) feature_bitfield |= CPU_FEATURE_SVE_ARM;
        #endif

        #ifdef HWCAP2_SVE2
            if (hwcap2 & HWCAP2_SVE2) feature_bitfield |= CPU_FEATURE_SVE2_ARM;
        #endif

        #ifdef HWCAP2_SVEPMULL
            if (hwcap2 & HWCAP2_SVEPMULL) feature_bitfield |= CPU_FEATURE_SVEPMULL_ARM;
        #endif

        (void) hwcap2;

        return feature_bitfield;
    }

    /**
     * Reads a number of /sys/devices/system/cpu/cpu0/cache/index<level>/<name> (K + M suffix supported)
     *
     * @return uint32 0 if the file doesn't exist
     */
    static
    uint32 cpu_info_cache_value(byte level, const char* name) NO_EXCEPT
    {
        char path[128];
        sprintf_fast(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/%s", (int32) level, name);

        if (!file_exists(path)) {
            return 0;
        }

        char buffer[32] = {};
        FileBody file = {};
        file.content = (byte *) buffer;
        file.size = sizeof(buffer) - 1;
        file_read(path, &file);

        const char* pos = buffer;
        uint32 value = (uint32) str_to_int(pos, &pos);
        if (*pos == 'K') {
            value *= 1024;
        } else if (*pos == 'M') {
            value *= 1024 * 1024;
        }

        return value;
    }

    /**
     * @param byte level Cache index (same as the x86 version): 0 = L1 data, 1 = L1 instruction, 2 = L2, 3 = L3
     */
    void cpu_info_cache(byte level, CpuCacheInfo* cache) NO_EXCEPT
    {
        cache->level = level;
        cache->partitions = 1;
        cache->size = cpu_info_cache_value(level, "size");
        cache->ways = (byte) cpu_info_cache_value(level, "ways_of_associativity");
        cache->sets = cpu_info_cache_value(level, "number_of_sets");
        cache->line_size = (uint16) cpu_info_cache_value(level, "coherency_line_size");
    }
#endif

#endif
