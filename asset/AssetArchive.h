/**
 * Jingga
 *
 * @copyright Jingga
 * @license    License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ASSET_ARCHIVE_H
#define COMS_ASSET_ARCHIVE_H

#include "../stdlib/Stdlib.h"
#include "../system/FileUtils.h"
#include "AssetType.h"

#define ASSET_ARCHIVE_VERSION 1

struct AssetArchiveElement {
    // NOTE: Why are we not using AssetType as data type?
    //      The problem is we rely on this for the archive_builder.cpp
    //      The program uses sizeof(AssetArchiveElement) for iteration
    //      Sure we could change it to a byte type but due to padding sizeof(...) would remain at the same size
    //      This would result in sizeof(...) != written data size to the file

    // AssetType type;
    uint32 type;

    uint32 start;
    uint32 length;
    uint32 uncompressed;

    // actual index for asset_dependencies
    // @question sometimes dependencies are in different files, this might be better as an id?
    uint32 dependency_start;
    uint32 dependency_count;
};

// It is important to understand that for performance reasons the assets addresses are stored in an array
// This makes it very fast to access because there is only one indirection.
// On the other hand we can only find assets by their ID/location and not by name.
struct AssetArchiveHeader {
    int32 version;

    uint32 asset_count;
    uint32 asset_dependency_count;

    AssetArchiveElement* asset_element; // is not the owner of the data
    int32* asset_dependencies; // is not the owner of the data
};

struct AssetArchive {
    AssetArchiveHeader header;
    byte* data; // owner of the data

    FileHandle fd;
    FileHandle fd_async;

    // @performance We still need to implement the loading with this and then profile it to see if it is faster.
    // If not remove
    MMFHandle mmf;

    // This is used to tell the asset archive in which AssetManagementSystem (AMS) which asset type is located.
    // Remember, many AMS only contain one asset type (e.g. image, audio, ...)
    // The reason for that is that different asset types need different chunk sizes
    byte asset_type_map[ASSET_TYPE_SIZE];
};

#endif