/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_TEMPLATE_H
#define COMS_HTML_TEMPLATE_H

#include "../../stdlib/Types.h"
#include "../../stdlib/PerfectHashMap.h"
#include "../../memory/RingMemory.h"
#include "../../system/FileUtils.cpp"

static
void template_find(const char* path, va_list args) {
    char** paths = va_arg(args, char**);
    uint32* path_count = va_arg(args, uint32*);
    uint32* max_path_count = va_arg(args, uint32*);
    uint32* total_file_size = va_arg(args, uint32*);
    RingMemory* ring = va_arg(args, RingMemory*);

    if (path_count == max_path_count) {
        uint32 old_max_path_count = *max_path_count;

        *max_path_count += 1000;
        char* new_paths = (char *) ring_get_memory(ring, (*max_path_count) * 256 * sizeof(char), 8, true);
        memcpy(new_paths, *paths, old_max_path_count * 256 * sizeof(char));
        *paths = new_paths;
    }

    *total_file_size += file_size(path);
    str_copy_short(paths[*path_count], path, 256);
    ++(*path_count);
}

void template_cache_alloc(
    PerfectHashMapRef* cache,
    const char* basedir,
    const char* file_ending,
    RingMemory* ring
) {
    // @todo limit the maximum cache size in the dynamic resize

    // the layout of the cache is as follows:
    //      * Perfect hash map memory which contains the offsets into this cache where the root AST node per template can be found (hash_entries)
    //      * Per template memory:
    //          * minified template string (64 byte aligned)
    //          * AST, with it'

    uint32 max_path_count = 1000;
    uint32 path_count = 0;
    char* paths = (char *) ring_get_memory(ring, max_path_count * 256 * sizeof(char), 8, true);
    uint32 total_file_size = 0;

    iterate_directory(basedir, file_ending, template_find, &paths, &path_count, &max_path_count, &total_file_size, ring);

    perfect_hashmap_alloc(
        cache,
        path_count,
        OMS_MAX((uint64) (total_file_size * 1.2f), (uint64) (total_file_size + 1 * KILOBYTE))
    );

    perfect_hashmap_prepare(&cache->hm, (const char*) paths, path_count, 256, 10000, ring);

    LOG_1(
        "Created template cache for %n templates with %n B in uncompressed file size",
        {
            {LOG_DATA_INT32, &path_count},
            {LOG_DATA_INT32, &total_file_size}
        }
    );
}

#endif