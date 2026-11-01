/**
 * Jingga
 *
 * @copyright Jingga
 * @license    License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ASSET_ARCHIVE_H
#define COMS_ASSET_ARCHIVE_H

#include "../stdlib/Stdlib.h"
#include "../system/FileUtils.h"
#include "AssetType.h"

#define ASSET_ARCHIVE_VERSION 1

/**
 * Asset archive (.asset), version ASSET_ARCHIVE_VERSION - binary only
 *
 * An archive is a flat blob of pre-processed assets: a small index (header), the
 * dependency table and the asset payloads, which are stored back to back.
 * There is no name table, assets are addressed by their numeric id
 *
 *      asset id = archive_asset_index | (archive_id << 24)
 *
 * @see ASSET_ID_FROM_ARCHIVE_AND_ASSET, the archive_id is the first line of the
 * .cfg file the archive was built from (ArchiveBuilder), it is also part of the
 * generated asset id header (<archive>.h, enum AssetArchiveIds<Name>).
 *
 * File layout (little endian, no padding, no magic, no checksum; the byte layout of
 * the payloads is described in the header of the respective asset type)
 *
 *      offset              size        type                        field
 *      0                   4           int32                       version (ASSET_ARCHIVE_VERSION)
 *      4                   4           uint32                      asset_count = N
 *      8                   4           uint32                      asset_dependency_count = D
 *      12                  24 * N      AssetArchiveElement[N]      index (element i = asset id i)
 *      12 + 24*N           4 * D       uint32[D]                   dependencies (asset ids)
 *      12 + 24*N + 4*D     ...         byte[]                      payloads
 *
 * AssetArchiveElement (24 bytes, six uint32 -> no padding)
 *
 *      offset  size    type    field
 *      0       4       uint32  type                AssetType (@see AssetType.h)
 *      4       4       uint32  start               file offset of the payload
 *      8       4       uint32  length              payload size in the file (encoded/compressed)
 *      12      4       uint32  uncompressed        size of the decoded data
 *      16      4       uint32  dependency_start    first index into the dependency array
 *      20      4       uint32  dependency_count    amount of dependencies of this asset
 *
 * The dependency array stores asset ids, not indices, so a dependency can live in
 * another archive (the high byte of the id is the archive id).
 *
 * The header is variable sized: read the first 12 bytes, then
 * 12 + 24*N + 4*D bytes (@see asset_archive_header_size) to navigate the file.
 * The payloads start directly behind the dependency array, 'start' is an absolute
 * offset. Payloads are written in asset order, the dependencies of an asset (e.g.
 * the texture of a font/atlas) are appended as regular assets right after it.
 *
 * Payload encoding per AssetType
 *      GENERAL         LZ4 block (shaders are pre-optimized before compression)
 *      OBJ             Mesh data (mesh_to_data)
 *      AUDIO           Audio header + QOA
 *      LANGUAGE        Language memory image (@see Language.h)
 *      FONT            Font binary (@see Font.h)
 *      TEXTURE_ATLAS   TextureAtlas binary (@see TextureAtlas.h)
 *      THEME           UITheme binary (@see UITheme.h)
 *      IMAGE           Image header + QOI
 *
 * Text side: the archive itself has no text representation. The builder input is a
 * .cfg table of contents (first line = archive id, one asset path per line,
 * relative to the .cfg), e.g.
 *
 *      4
 *      ./ui_font.fonttxt
 *      ./img/ui_atlas.atlastxt
 *      ./img/ui_bg_atlas.png
 *
 * which produces <name>.asset (this format) and <name>.h (the asset id enum).
 */
struct AssetArchiveElement {
    // NOTE: Why are we not using AssetType as data type?
    //      The problem is we rely on this for the archive_builder.cpp
    //      The program uses sizeof(AssetArchiveElement) for iteration
    //      Sure we could change it to a byte type but due to padding sizeof(...) would remain at the same size
    //      This would result in sizeof(...) != written data size to the file

    // @type AssetType
    uint32 type;

    uint32 start;
    uint32 length;
    uint32 uncompressed;

    // actual index for asset_dependencies
    // These values link into the "global"/archive dependency list
    // We store all dependencies in a continuous array
    // but for a single element of course we only need a small subsection
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

    // How many total dependencies are defined in this archive
    uint32 asset_dependency_count;

    AssetArchiveElement* asset_element; // is not the owner of the data

    // The individual elements link into this array
    uint32* asset_dependencies; // is not the owner of the data
};

/**
 * In-memory archive
 *
 * The file only stores the three scalars + the two arrays of AssetArchiveHeader
 * (that is why this struct must not be written to a file as a whole), the
 * pointers point into data, which is the owner of the file content.
 */
struct AssetArchive {
    AssetArchiveHeader header;

    size_t data_size;
    byte* data; // owner of the data

    FileHandle fd;
    FileHandle fd_async;

    // @performance We still need to implement the loading with this and then profile it to see if it is faster.
    // If not remove
    MMFHandle mmf;
};

#endif