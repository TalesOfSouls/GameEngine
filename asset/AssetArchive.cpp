/**
 * Jingga
 *
 * @copyright Jingga
 * @license    License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ASSET_ARCHIVE_C
#define COMS_ASSET_ARCHIVE_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../utils/Utils.h"
#include "../memory/ChunkMemory.cpp"
#include "../image/Image.cpp"
#include "../image/Qoi.h"
#include "../object/Mesh.cpp"
#include "../object/Texture.h"
#include "../object/TextureAtlas.cpp"
#include "../audio/Audio.cpp"
#include "../audio/Qoa.h"
#include "../font/Font.cpp"
#include "../localization/Language.cpp"
#include "../ui/UITheme.cpp"
#include "../compression/LZ4.h"
#include "Asset.h"
#include "AssetArchive.h"
#include "AssetManagementSystem.cpp"
#include "../system/FileUtils.cpp"

/**
 * We store the archive id in the asset id (1 byte)
 * This macro simplifies reading that archive id from the asset id
 */
#define ARCHIVE_ID_FROM_ASSET_ID(asset_id) (((asset_id) >> 24) & 0xFF)

/**
 * We store the archive id in the asset id (1 byte)
 * This macro simplifies reading the raw asset id without the archive id
 */
#define ASSET_RAW_ID_FROM_ID(asset_id) ((asset_id) & 0x00FFFFFF)

/**
 * Builds the asset id from the raw asset id and the archive id into one combined id
 */
#define ASSET_ID_FROM_ARCHIVE_AND_ASSET(raw_asset_id, archive_id) ((raw_asset_id) | ((archive_id) << 24))

/**
 * Calculates the header size.
 * In other words how much data you have to read from the archive file to completely parse the header
 * This includes all the data incl. the dependency array per element
 * Based on this information you can then navigate the archive file to find your assets
 */
static inline
int32 asset_archive_header_size(
    const AssetArchive* const __restrict archive,
    const byte* __restrict data
) NO_EXCEPT
{
    data += sizeof(archive->header.version);

    int32 asset_count;
    data = read_le(data, &asset_count);
    ASSERT_TRUE(asset_count > 0);

    int32 asset_dependency_count;
    read_le(data, &asset_dependency_count);

    ASSERT_TRUE(asset_count + asset_dependency_count < 100000);

    return sizeof(archive->header.version)
        + sizeof(archive->header.asset_count)
        + sizeof(archive->header.asset_dependency_count)
        + asset_count * sizeof(AssetArchiveElement)
        + asset_dependency_count * sizeof(int32);
}

static inline
void asset_archive_header_load(
    AssetArchiveHeader* const __restrict header,
    MAYBE_UNUSED size_t header_size,
    const byte* __restrict data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    data = read_le(data, &header->version);
    data = read_le(data, &header->asset_count);
    data = read_le(data, &header->asset_dependency_count);

    ASSERT_TRUE(
        header->asset_count * sizeof(AssetArchiveElement)
        + header->asset_dependency_count * sizeof(int32)
        <= header_size
    );
    PSEUDO_USE(header_size);

    memcpy(header->asset_element, data, header->asset_count * sizeof(AssetArchiveElement));
    data += header->asset_count * sizeof(AssetArchiveElement);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) header->asset_element,
        (int32 *) header->asset_element,
        (header->asset_count * sizeof(AssetArchiveElement)) / 4, // everything is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    // Load dependency data
    if (header->asset_dependency_count) {
        header->asset_dependencies = (uint32 *) (
            (byte *) header->asset_element
            + header->asset_count * sizeof(AssetArchiveElement)
        );

        memcpy(header->asset_dependencies, data, header->asset_dependency_count * sizeof(int32));
        SWAP_ENDIAN_LITTLE_SIMD(
            (int32 *) header->asset_dependencies,
            (int32 *) header->asset_dependencies,
            header->asset_count * header->asset_dependency_count, // everything is 4 bytes -> easy to swap
            steps
        );
    }
}

FORCE_INLINE
const AssetArchiveElement* asset_archive_element_find(const AssetArchive* archive, int32 id) NO_EXCEPT
{
    return &archive->header.asset_element[id];
}

static CONSTEXPR inline
uint32 asset_type_size(int32 type) NO_EXCEPT
{
    switch (type) {
        case ASSET_TYPE_GENERAL:
            return 0;
        case ASSET_TYPE_AUDIO:
            return sizeof(Audio);
        case ASSET_TYPE_FONT:
            return sizeof(Font);
        case ASSET_TYPE_IMAGE:
            return sizeof(Image);
        case ASSET_TYPE_TEXTURE_ATLAS:
            return sizeof(TextureAtlas);
        case ASSET_TYPE_OBJ:
            return sizeof(Mesh);
        case ASSET_TYPE_LANGUAGE:
            return sizeof(Language);
        case ASSET_TYPE_THEME:
            return sizeof(UITheme);
        default:
            UNREACHABLE();
    }
}

static CONSTEXPR inline
uint32 asset_align_size(int32 type) NO_EXCEPT
{
    switch (type) {
        case ASSET_TYPE_GENERAL:
            return 0;
        case ASSET_TYPE_AUDIO:
            return alignof(Audio);
        case ASSET_TYPE_FONT:
            return alignof(Font);
        case ASSET_TYPE_IMAGE:
            return alignof(Image);
        case ASSET_TYPE_TEXTURE_ATLAS:
            return alignof(TextureAtlas);
        case ASSET_TYPE_OBJ:
            return alignof(Mesh);
        case ASSET_TYPE_LANGUAGE:
            return alignof(Language);
        case ASSET_TYPE_THEME:
            return alignof(UITheme);
        default:
            UNREACHABLE();
    }
}

// Sometimes our asset types have additional data that needs to be aligned
// Example Image has Image.pixels which has to be at least 4 byte aligned
static CONSTEXPR inline
uint32 asset_data_align_size(int32 type) NO_EXCEPT
{
    switch (type) {
        case ASSET_TYPE_GENERAL:
            return alignof(size_t);
        case ASSET_TYPE_AUDIO:
            return alignof(size_t);
        case ASSET_TYPE_FONT:
            return alignof(size_t);
        case ASSET_TYPE_IMAGE:
            return 64; // 64 bytes so we can use AVX512 on pixels
        case ASSET_TYPE_TEXTURE_ATLAS:
            return 64; // 64 bytes so we can use AVX512 on uv data
        case ASSET_TYPE_OBJ:
            return alignof(size_t);
        case ASSET_TYPE_LANGUAGE:
            return alignof(size_t);
        case ASSET_TYPE_THEME:
            return alignof(size_t);
        default:
            UNREACHABLE();
    }
}

/**
 * Asset archives files remain open from the _load() function
 * They need to be explicitly closed when no longer needed.
 *
 * @param AssetArchive* archive Archive
 *
 * @return void
 */
inline
void asset_archive_close(AssetArchive* const archive) {
    file_close_handle(archive->fd);
    file_close_handle(archive->fd_async);
    file_mmf_close(archive->mmf);
}

void asset_archive_load(
    AssetArchive* archive,
    wchar_t* path,
    BufferMemory* const mem,
    int32 steps = 8
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_ASSET_ARCHIVE_LOAD, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);

    LOG_1("[INFO] Load AssetArchive");

    archive->fd = file_read_handle(path);
    if (!archive->fd) {
        ASSERT_THROW();
        return;
    }

    archive->fd_async = file_read_async_handle(path);
    if (!archive->fd_async) {
        ASSERT_THROW();
        return;
    }
    archive->mmf = file_mmf_handle(archive->fd_async);

    FileBody file = {0};

    // We only want to read the header at first
    file.size = sizeof(AssetArchiveHeader)
        + MEMBER_SIZEOF(AssetArchiveHeader, version)
        + MEMBER_SIZEOF(AssetArchiveHeader, asset_count)
        + MEMBER_SIZEOF(AssetArchiveHeader, asset_dependency_count);

    BUFFER_STACK_MEMORY_START(mem);
    // Find header size (+ 1 to later store \0)
    file.content = memory_get(mem, file.size + 1, alignof(size_t));

    file_read(archive->fd, &file, 0, file.size);
    file.size = asset_archive_header_size(archive, file.content);

    // WARNING archive->data needs to be already allocated
    ASSERT_TRUE(archive->data);
    archive->header.asset_element = (AssetArchiveElement *) archive->data;

    // Reset file position
    file_seek(archive->fd, 0);

    // Read entire header
    file.content = memory_get(mem, file.size, sizeof(size_t));
    file_read(archive->fd, &file, 0, file.size);
    asset_archive_header_load(&archive->header, archive->data_size, file.content, steps);

    LOG_1(
        "[INFO] Loaded AssetArchive %s with %d assets",
        {DATA_TYPE_CHAR_STR, (void *) path}, {DATA_TYPE_UINT32, (void *) &archive->header.asset_count}
    );
}

// @bug I'm afraid that loading the same asset twice could result in circumstances where it gets added twice
Asset* const asset_archive_asset_load(
    const AssetArchive* const archive,
    int32 id,
    AssetManagementSystem* const ams,
    ThrdChunkMemory* const mem,
    bool load_dependencies = true
) NO_EXCEPT
{
    // Create a string representation from the asset id
    // We can't just use the asset id, since an int can have a \0 between high byte and low byte
    // @question We maybe can switch the AMS to work with ints as keys.
    //          We would then have to also create an application specific enum for general assets,
    //          that are not stored in the asset archive (e.g. color palette, which is generated at runtime).
    char id_str[9];
    int_to_hex(id, id_str);

    PROFILE_DEBUG(PROFILE_ASSET_ARCHIVE_ASSET_LOAD, id_str, PROFILE_FLAG_SHOULD_LOG);

    // Check if asset already exists
    Asset* asset = ams_asset_get_wait(ams, id_str);
    if (asset) {
        // Prevent garbage collection
        asset->state &= ~ASSET_MEMORY_STATE_RAM_GC;
        asset->state &= ~ASSET_MEMORY_STATE_VRAM_GC;

        return asset;
    }

    const AssetArchiveElement* const element = &archive->header.asset_element[ASSET_RAW_ID_FROM_ID(id)];

    ASSERT_TRUE(element->type < ASSET_TYPE_SIZE);
    ASSERT_TRUE(element->uncompressed > 0);

    LOG_2(
        "[INFO] Load asset %d from archive %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_UINT64, &id},
        {DATA_TYPE_UINT32, &element->type},
        {DATA_TYPE_UINT32, &element->length},
        {DATA_TYPE_UINT32, &element->uncompressed}
    );

    /**
     * This determins how the data is loaded, decompressed and possibly stored into objects
     */
    // We are reading into temp memory since we have to perform transformations/decompression on the data
    FileBodyAsync file = {0};
    THRD_CHUNK_STACK_MEMORY(mem, &file.content, element->length + 1);
    file_read_async(archive->fd_async, &file, element->start, element->length);

    const size_t asset_memory_requirement = element->uncompressed // This is the data that gets stored in our specialized asset struct (e.g. .pixels in Image)
        + asset_type_size(element->type) // This is the specialized asset struct (e.g. Image, TextureAtlas, ...)
        + asset_align_size(element->type) // This is how we need to align our asset struct
        + asset_data_align_size(element->type); // This is how we need to align our data in the asset struct (e.g. .pixels in Image)

    // This happens while the file system loads the data
    // The important part is to reserve the uncompressed file size, not the compressed one
    // @bug Don't I have to mark_completed the underlying thread_chunk element? see code of function
    asset = ams_asset_reserve(
        ams,
        id_str,
        (uint32) asset_memory_requirement
    );
    asset->official_id = id;

    ASSERT_TRUE(asset_memory_requirement < asset->ram_size);

    asset->data_size = element->uncompressed;
    asset->state |= ASSET_MEMORY_STATE_IN_RAM;

    file_async_wait(archive->fd_async, &file.ov, true);

    // @bug Couldn't the asset become available from ams_asset_get_wait to here?
    // This would mean we are overwriting it
    // A solution could be a function called ams_get_reserve_wait() that reserves, if not available
    // However, that function would have to lock the ams during that entire time
    switch (element->type) {
        case ASSET_TYPE_GENERAL: {
            byte* const raw_data = asset->self;

            lz4_decode(file.content, file.size, raw_data);
        } break;
        case ASSET_TYPE_TEXTURE_ATLAS: {
            TextureAtlas* const atlas = (TextureAtlas *) asset->self;
            atlas->elements = (TextureAtlasElement *) (atlas + 1);

            atlas_from_data(file.content, atlas);
        } break;
        case ASSET_TYPE_IMAGE: {
            Texture* texture = (Texture *) asset->self;
            texture->image.pixels = (byte *) align_up((uintptr_t) (texture + 1), 64);

            file.content += image_header_from_data(file.content, &texture->image);
            qoi_decode(file.content, &texture->image);

            asset->vram_size = texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings);
        } break;
        case ASSET_TYPE_AUDIO: {
            Audio* const audio = (Audio *) asset->self;
            audio->data = (byte *) (audio + 1);

            file.content += audio_header_from_data(file.content, audio);
            qoa_decode(file.content, audio);
        } break;
        case ASSET_TYPE_OBJ: {
            Mesh* const mesh = (Mesh *) asset->self;
            mesh->data = (byte *) (mesh + 1);

            mesh_from_data(file.content, mesh);
        } break;
        case ASSET_TYPE_LANGUAGE: {
            Language* const language = (Language *) asset->self;
            language->data = (byte *) (language + 1);

            language_from_data(file.content, language);
        } break;
        case ASSET_TYPE_FONT: {
            Font* const font = (Font *) asset->self;
            font->glyphs = (Glyph *) (font + 1);

            font_from_data(file.content, font);
        } break;
        case ASSET_TYPE_THEME: {
            UITheme* const theme = (UITheme *) asset->self;
            theme->data = (byte *) (theme + 1);

            theme_from_data(file.content, theme);
        } break;
        default: {
            UNREACHABLE();
        }
    }

    // Even though dependencies are still being loaded
    // the main program should still be able to do some work if possible
    ams_set_loaded(ams, asset);

    LOG_2(
        "[INFO] Loaded asset %d from archive %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_UINT64, &id},
        {DATA_TYPE_UINT32, &element->type},
        {DATA_TYPE_UINT32, &element->length},
        {DATA_TYPE_UINT32, &element->uncompressed}
    );

    if (element->dependency_count) {
        asset->reference_count = (uint16) element->dependency_count;
        memcpy(
            asset->references,
            &archive->header.asset_dependencies[element->dependency_start],
            sizeof(uint32)
        );

        // @performance maybe do in worker threads or AppCmdbuffer? This just feels very slow
        //          Careful if we do this threaded we cannot use memory_get_temp() above!
        if (load_dependencies) {
            for (uint32 i = 0; i < element->dependency_count; ++i) {
                asset_archive_asset_load(archive, asset->references[i], ams, mem);
            }
        }
    }

    return asset;
}

#endif