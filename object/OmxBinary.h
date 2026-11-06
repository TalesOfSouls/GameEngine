/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_OMX_BINARY_H
#define COMS_OBJECT_OMX_BINARY_H

#include "../stdlib/Stdlib.h"
#include "Omx.h"

// Release format
//      [OmxFile header with pointers replaced by offsets][data block]
// Since all data is already in one block (OmxFile::data) the binary file is just a copy of it.
// Loading = read file + fix up pointers (no parsing, no allocation besides the file itself)
// @todo Big endian platforms (all values are little endian)

#define OMX_BINARY_MAGIC 0x31584D4F // "OMX1"

#define OMX_POINTER_LIST(X) \
    X(assets)               \
    X(strings)              \
    X(materials)            \
    X(skeletons)            \
    X(bones)                \
    X(animations)           \
    X(channels)             \
    X(keys)                 \
    X(events)               \
    X(hitboxes)             \
    X(shapes)               \
    X(particle_systems)     \
    X(vertices)             \
    X(indices)              \
    X(meshes)               \
    X(slots)                \
    X(sockets)              \
    X(emitters)             \
    X(state_fields)

struct OmxBinaryHeader {
    uint32 magic;
    uint32 header_size; // = sizeof(OmxBinaryHeader), used as compatibility check
    uint64 data_size;
};

// The data block starts aligned after the header -> the file can be used in place
#define OMX_BINARY_DATA_OFFSET ((sizeof(OmxBinaryHeader) + sizeof(OmxFile) + (OMX_MEMORY_ALIGNMENT - 1)) & ~((uint64) OMX_MEMORY_ALIGNMENT - 1))

inline
uint64 omx_binary_size(const OmxFile* file) NO_EXCEPT
{
    return OMX_BINARY_DATA_OFFSET + file->data_size;
}

// out must be at least omx_binary_size() bytes
uint64 omx_to_binary(const OmxFile* file, byte* out) NO_EXCEPT
{
    OmxBinaryHeader header = { OMX_BINARY_MAGIC, (uint32) sizeof(OmxBinaryHeader), file->data_size };
    memcpy(out, &header, sizeof(header));

    // Pointers -> offsets (+1 so that NULL stays NULL)
    OmxFile copy = *file;
    copy.data = NULL;

    #define OMX_TO_OFFSET(name) \
        *((uintptr_t *) &copy.name) = file->name ? (uintptr_t) ((byte *) file->name - file->data) + 1 : 0;

    OMX_POINTER_LIST(OMX_TO_OFFSET)
    #undef OMX_TO_OFFSET

    memcpy(out + sizeof(header), &copy, sizeof(copy));
    memset(out + sizeof(header) + sizeof(copy), 0, OMX_BINARY_DATA_OFFSET - sizeof(header) - sizeof(copy));
    memcpy(out + OMX_BINARY_DATA_OFFSET, file->data, file->data_size);

    return omx_binary_size(file);
}

// The OmxFile points into binary afterwards -> binary must stay alive and must be OMX_MEMORY_ALIGNMENT aligned
bool omx_from_binary(byte* binary, uint64 size, OmxFile* file) NO_EXCEPT
{
    if (size < OMX_BINARY_DATA_OFFSET) {
        return false;
    }

    OmxBinaryHeader header;
    memcpy(&header, binary, sizeof(header));

    if (header.magic != OMX_BINARY_MAGIC
        || header.header_size != sizeof(OmxBinaryHeader)
        || OMX_BINARY_DATA_OFFSET + header.data_size > size
    ) {
        return false;
    }

    memcpy(file, binary + sizeof(header), sizeof(OmxFile));
    file->data = binary + OMX_BINARY_DATA_OFFSET;
    file->data_size = header.data_size;

    #define OMX_TO_POINTER(name)                                         \
        {                                                                \
            const uintptr_t offset = *((uintptr_t *) &file->name);       \
            if (offset > file->data_size + 1) {                          \
                return false;                                            \
            }                                                            \
                                                                         \
            *((byte **) &file->name) = offset ? file->data + offset - 1 : NULL; \
        }

    OMX_POINTER_LIST(OMX_TO_POINTER)
    #undef OMX_TO_POINTER

    return true;
}

#endif
