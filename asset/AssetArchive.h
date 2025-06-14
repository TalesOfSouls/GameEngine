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

#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"
#include "../utils/EndianUtils.h"
#include "../utils/Utils.h"
#include "../memory/RingMemory.h"
#include "../memory/BufferMemory.h"
#include "../image/Image.cpp"
#include "../image/Qoi.h"
#include "../object/Mesh.h"
#include "../object/Texture.h"
#include "../audio/Audio.cpp"
#include "../audio/Qoa.h"
#include "../font/Font.h"
#include "../localization/Language.h"
#include "../ui/UITheme.h"
#include "AssetManagementSystem.h"
#include "../system/FileUtils.cpp"
#include "../stdlib/Simd.h"
#include "../compiler/CompilerUtils.h"

#define ASSET_ARCHIVE_VERSION 1

struct AssetArchiveElement {
    uint32 type;

    uint32 start;
    uint32 length;
    uint32 uncompressed;

    uint32 dependency_start; // actual index for asset_dependencies
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
    byte asset_type_map[ASSET_TYPE_SIZE];
};

// Calculates how large the header memory has to be to hold all its information
int32 asset_archive_header_size(AssetArchive* __restrict archive, const byte* __restrict data)
{
    data += sizeof(archive->header.version);

    int32 asset_count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(archive->header.asset_count);

    int32 asset_dependency_count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(archive->header.asset_dependency_count);

    return sizeof(archive->header.version)
        + sizeof(archive->header.asset_count)
        + sizeof(archive->header.asset_dependency_count)
        + asset_count * sizeof(AssetArchiveElement)
        + asset_dependency_count * sizeof(int32);
}

void asset_archive_header_load(AssetArchiveHeader* __restrict header, const byte* __restrict data, [[maybe_unused]] int32 steps = 8)
{
    header->version = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(header->version);

    header->asset_count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(header->asset_count);

    header->asset_dependency_count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(header->asset_dependency_count);

    memcpy(header->asset_element, data, header->asset_count * sizeof(AssetArchiveElement));
    data += header->asset_count * sizeof(AssetArchiveElement);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) header->asset_element,
        (int32 *) header->asset_element,
        header->asset_count * sizeof(AssetArchiveElement) / 4, // everything is 4 bytes -> easy to swap
        steps
    );

    if (header->asset_dependency_count) {
        header->asset_dependencies = (int32 *) ((byte *) header->asset_element + header->asset_count * sizeof(AssetArchiveElement));
    }

    memcpy(header->asset_dependencies, data, header->asset_dependency_count * sizeof(int32));
    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) header->asset_dependencies,
        (int32 *) header->asset_dependencies,
        header->asset_count * header->asset_dependency_count, // everything is 4 bytes -> easy to swap
        steps
    );
}

inline
AssetArchiveElement* asset_archive_element_find(const AssetArchive* archive, int32 id)
{
    return &archive->header.asset_element[id];
}

void asset_archive_load(AssetArchive* archive, const char* path, BufferMemory* buf, RingMemory* ring, int32 steps = 8)
{
    PROFILE(PROFILE_ASSET_ARCHIVE_LOAD, path, false, true);

    LOG_1(
        "Load AssetArchive %s",
        {{LOG_DATA_CHAR_STR, (void *) path}}
    );

    archive->fd = file_read_handle(path);
    if (!archive->fd) {
        return;
    }

    archive->fd_async = file_read_async_handle(path);
    if (!archive->fd_async) {
        return;
    }
    archive->mmf = file_mmf_handle(archive->fd_async);

    FileBody file = {};
    file.size = 64;

    // Find header size
    file.content = ring_get_memory(ring, file.size, 4);
    file_read(archive->fd, &file, 0, file.size);
    file.size = asset_archive_header_size(archive, file.content);

    // Reserve memory for the header
    archive->data = buffer_get_memory(
        buf,
        file.size
            - sizeof(archive->header.version)
            - sizeof(archive->header.asset_count)
            - sizeof(archive->header.asset_dependency_count),
        4
    );

    archive->header.asset_element = (AssetArchiveElement *) archive->data;

    // Read entire header
    file.content = ring_get_memory(ring, file.size);
    file_read(archive->fd, &file, 0, file.size);
    asset_archive_header_load(&archive->header, file.content, steps);

    LOG_1(
        "Loaded AssetArchive %s with %d assets",
        {{LOG_DATA_CHAR_STR, (void *) path}, {LOG_DATA_UINT32, (void *) &archive->header.asset_count}}
    );
}

// @question Do we want to allow a callback function?
// Very often we want to do something with the data (e.g. upload it to the gpu)
// Maybe we could just accept a int value which we set atomically as a flag that the asset is complete?
// this way we can check much faster if we can work with this data from the caller?!
// The only problem is that we need to pass the pointer to this int in the thrd_queue since we queue the files to load there
Asset* asset_archive_asset_load(const AssetArchive* archive, int32 id, AssetManagementSystem* ams, RingMemory* ring)
{
    // Create a string representation from the asset id
    // We can't just use the asset id, since an int can have a \0 between high byte and low byte
    // @question We maybe can switch the AMS to work with ints as keys.
    // We would then have to also create an application specific enum for general assets,
    // that are not stored in the asset archive (e.g. color palette, which is generated at runtime).
    char id_str[9];
    int_to_hex(id, id_str);

    PROFILE(PROFILE_ASSET_ARCHIVE_ASSET_LOAD, id_str, false, true);
    // @todo add calculation from element->type to ams index. Probably requires an app specific conversion function

    // We have to mask 0x00FFFFFF since the highest bits define the archive id, not the element id
    AssetArchiveElement* element = &archive->header.asset_element[id & 0x00FFFFFF];

    byte component_id = archive->asset_type_map[element->type];
    //AssetComponent* ac = &ams->asset_components[component_id];

    LOG_2(
        "Load asset %d from archive %d for AMS %d with %n B compressed and %n B uncompressed",
        {{LOG_DATA_UINT64, &id}, {LOG_DATA_UINT32, &element->type}, {LOG_DATA_BYTE, &component_id}, {LOG_DATA_UINT32, &element->length}, {LOG_DATA_UINT32, &element->uncompressed}}
    );

    Asset* asset = thrd_ams_get_asset_wait(ams, id_str);

    if (asset) {
        // Prevent garbage collection
        asset->state &= ~ASSET_STATE_RAM_GC;
        asset->state &= ~ASSET_STATE_VRAM_GC;

        return asset;
    }

    // @bug Couldn't the asset become available from thrd_ams_get_asset_wait to here?
    // This would mean we are overwriting it
    // A solution could be a function called thrd_ams_get_reserve_wait() that reserves, if not available
    // However, that function would have to lock the ams during that entire time

    if (element->type == 0) {
        asset = thrd_ams_reserve_asset(ams, (byte) component_id, id_str, element->uncompressed);
        asset->official_id = id;
        asset->ram_size = element->uncompressed;

        FileBody file = {};
        file.content = asset->self;

        // @performance Consider to implement general purpose fast compression algorithm

        // We are directly reading into the correct destination
        file_read(archive->fd, &file, element->start, element->length);
    } else {
        // @performance In this case we may want to check if memory mapped regions are better.
        // 1. I don't think they work together with async loading
        // 2. Profile which one is faster
        // 3. The big benefit of mmf would be that we can avoid one memcpy and directly load the data into the object
        // 4. Of course the disadvantage would be to no longer have async loading

        // We are reading into temp memory since we have to perform transformations on the data
        FileBodyAsync file = {};
        file_read_async(archive->fd_async, &file, element->start, element->length, ring);

        // This happens while the file system loads the data
        // The important part is to reserve the uncompressed file size, not the compressed one
        asset = thrd_ams_reserve_asset(ams, (byte) component_id, id_str, element->uncompressed);
        asset->official_id = id;

        asset->state |= ASSET_STATE_IN_RAM;

        file_async_wait(archive->fd_async, &file.ov, true);
        switch (element->type) {
            case ASSET_TYPE_IMAGE: {
                // @todo Do we really want to store textures in the asset management system or only images?
                // If it is only images then we need to somehow also manage textures
                Texture* texture = (Texture *) asset->self;
                texture->image.pixels = (byte *) (texture + 1);

                qoi_decode(file.content, &texture->image);

                asset->vram_size = texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings);
                asset->ram_size = asset->vram_size + sizeof(Texture);

                #if OPENGL || VULKAN
                    // If opengl, we always flip
                    if (!(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)) {
                        image_flip_vertical(ring, &texture->image);
                    }
                #endif
            } break;
            case ASSET_TYPE_AUDIO: {
                Audio* audio = (Audio *) asset->self;
                audio->data = (byte *) (audio + 1);

                qoa_decode(file.content, audio);
            } break;
            case ASSET_TYPE_OBJ: {
                Mesh* mesh = (Mesh *) asset->self;
                mesh->data = (byte *) (mesh + 1);

                mesh_from_data(file.content, mesh);
            } break;
            case ASSET_TYPE_LANGUAGE: {
                Language* language = (Language *) asset->self;
                language->data = (byte *) (language + 1);

                language_from_data(file.content, language);
            } break;
            case ASSET_TYPE_FONT: {
                Font* font = (Font *) asset->self;
                font->glyphs = (Glyph *) (font + 1);

                font_from_data(file.content, font);
            } break;
            case ASSET_TYPE_THEME: {
                UIThemeStyle* theme = (UIThemeStyle *) asset->self;
                theme->data = (byte *) (theme + 1);

                theme_from_data(file.content, theme);
            } break;
            default: {
                UNREACHABLE();
            }
        }
    }

    // Even though dependencies are still being loaded
    // the main program should still be able to do some work if possible
    thrd_ams_set_loaded(asset);

    LOG_2(
        "Loaded asset %d from archive %d for AMS %d with %n B compressed and %n B uncompressed",
        {{LOG_DATA_UINT64, &id}, {LOG_DATA_UINT32, &element->type}, {LOG_DATA_BYTE, &component_id}, {LOG_DATA_UINT32, &element->length}, {LOG_DATA_UINT32, &element->uncompressed}}
    );

    // @performance maybe do in worker threads? This just feels very slow
    // @bug dependencies might be stored in different archives?
    for (uint32 i = 0; i < element->dependency_count; ++i) {
        asset_archive_asset_load(archive, id, ams, ring);
    }

    return asset;
}

#endif