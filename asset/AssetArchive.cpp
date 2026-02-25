/**
 * Jingga
 *
 * @copyright Jingga
 * @license    License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ASSET_ARCHIVE_C
#define COMS_ASSET_ARCHIVE_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../utils/Utils.h"
#include "../memory/RingMemory.cpp"
#include "../memory/BufferMemory.h"
#include "../image/Image.cpp"
#include "../image/Qoi.h"
#include "../object/Mesh.cpp"
#include "../object/Texture.h"
#include "../audio/Audio.cpp"
#include "../audio/Qoa.h"
#include "../font/Font.cpp"
#include "../localization/Language.cpp"
#include "../ui/UITheme.cpp"
#include "AssetManagementSystem.cpp"
#include "../system/FileUtils.cpp"
#include "../compiler/CompilerUtils.h"
#include "Asset.h"
#include "AssetArchive.h"

// Calculates how large the header memory has to be to hold all its information
static inline
int32 asset_archive_header_size(
    const AssetArchive* const __restrict archive,
    const byte* __restrict data
) NO_EXCEPT
{
    data += sizeof(archive->header.version);

    int32 asset_count;
    data = read_le(data, &asset_count);

    int32 asset_dependency_count;
    read_le(data, &asset_dependency_count);

    //data += sizeof(archive->header.asset_dependency_count);

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
    const byte* __restrict data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    data = read_le(data, &header->version);
    data = read_le(data, &header->asset_count);
    data = read_le(data, &header->asset_dependency_count);

    memcpy(header->asset_element, data, header->asset_count * sizeof(AssetArchiveElement));
    data += header->asset_count * sizeof(AssetArchiveElement);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) header->asset_element,
        (int32 *) header->asset_element,
        header->asset_count * sizeof(AssetArchiveElement) / 4, // everything is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

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

FORCE_INLINE
AssetArchiveElement* asset_archive_element_find(const AssetArchive* archive, int32 id) NO_EXCEPT
{
    return &archive->header.asset_element[id];
}

static inline
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
        case ASSET_TYPE_OBJ:
            return sizeof(Mesh);
        case ASSET_TYPE_LANGUAGE:
            return sizeof(Language);
        case ASSET_TYPE_THEME:
            return sizeof(UIThemeStyle);
        default:
            UNREACHABLE();
    }
}

void asset_archive_load(
    AssetArchive* archive,
    const wchar_t* path,
    RingMemory* const ring,
    int32 steps = 8
) NO_EXCEPT
{
    PROFILE(PROFILE_ASSET_ARCHIVE_LOAD, NULL, PROFILE_FLAG_SHOULD_LOG);

    LOG_1("[INFO] Load AssetArchive");

    archive->fd = file_read_handle(path);
    if (!archive->fd) {
        return;
    }

    archive->fd_async = file_read_async_handle(path);
    if (!archive->fd_async) {
        return;
    }
    archive->mmf = file_mmf_handle(archive->fd_async);

    FileBody file = {0};

    // We only want to read the header at first
    file.size = sizeof(AssetArchiveHeader);

    // Find header size
    file.content = ring_get_memory(ring, file.size, sizeof(size_t));
    file_read(archive->fd, &file, 0, file.size);
    file.size = asset_archive_header_size(archive, file.content);

    // WARNING archive->data needs to be already allocated
    ASSERT_TRUE(archive->data);
    archive->header.asset_element = (AssetArchiveElement *) archive->data;

    // Reset file position
    file_seek(archive->fd, 0);

    // Read entire header
    file.content = ring_get_memory(ring, file.size, sizeof(size_t));
    file_read(archive->fd, &file, 0, file.size);
    asset_archive_header_load(&archive->header, file.content, steps);

    LOG_1(
        "[INFO] Loaded AssetArchive %s with %d assets",
        {DATA_TYPE_CHAR_STR, (void *) path}, {DATA_TYPE_UINT32, (void *) &archive->header.asset_count}
    );
}

// @question Do we want to allow a callback function?
// Very often we want to do something with the data (e.g. upload it to the gpu)
// Maybe we could just accept a int value which we set atomically as a flag that the asset is complete?
// this way we can check much faster if we can work with this data from the caller?!
// The only problem is that we need to pass the pointer to this int in the thrd_queue since we queue the files to load there
Asset* const asset_archive_asset_load(
    const AssetArchive* const archive,
    int32 id,
    AssetManagementSystem* const ams,
    RingMemory* const ring
) NO_EXCEPT
{
    // Create a string representation from the asset id
    // We can't just use the asset id, since an int can have a \0 between high byte and low byte
    // @question We maybe can switch the AMS to work with ints as keys.
    // We would then have to also create an application specific enum for general assets,
    // that are not stored in the asset archive (e.g. color palette, which is generated at runtime).
    char id_str[9];
    int_to_hex(id, id_str);

    PROFILE(PROFILE_ASSET_ARCHIVE_ASSET_LOAD, id_str, PROFILE_FLAG_SHOULD_LOG);
    // @todo add calculation from element->type to ams index. Probably requires an app specific conversion function

    // We have to mask 0x00FFFFFF since the highest bits define the archive id, not the element id
    const AssetArchiveElement* const element = &archive->header.asset_element[id & 0x00FFFFFF];

    ASSERT_TRUE(element->type < ASSET_TYPE_SIZE);
    byte component_id = archive->asset_type_map[element->type];
    //AssetComponent* ac = &ams->asset_components[component_id];

    LOG_2(
        "[INFO] Load asset %d from archive %d for AMS %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_UINT64, &id}, {DATA_TYPE_UINT32, &element->type}, {DATA_TYPE_UINT8, &component_id}, {DATA_TYPE_UINT32, &element->length}, {DATA_TYPE_UINT32, &element->uncompressed}
    );

    // Check if asset already exists
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
    if (element->type == ASSET_TYPE_GENERAL) {
        asset = thrd_ams_reserve_asset(ams, (byte) component_id, id_str, element->uncompressed);
        asset->official_id = id;
        asset->ram_size = element->uncompressed;

        FileBody file = {0};
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
        FileBodyAsync file = {0};
        file_read_async(archive->fd_async, &file, element->start, element->length, ring);

        // This happens while the file system loads the data
        // The important part is to reserve the uncompressed file size, not the compressed one
        asset = thrd_ams_reserve_asset(ams, (byte) component_id, id_str, element->uncompressed + asset_type_size(element->type));
        asset->official_id = id;

        asset->state |= ASSET_STATE_IN_RAM;

        file_async_wait(archive->fd_async, &file.ov, true);
        switch (element->type) {
            case ASSET_TYPE_IMAGE: {
                // @todo Do we really want to store textures in the asset management system or only images?
                // If it is only images then we need to somehow also manage textures
                Texture* texture = (Texture *) asset->self;
                texture->image.pixels = (byte *) (texture + 1);

                file.content += image_header_from_data(file.content, &texture->image);
                qoi_decode(file.content, &texture->image);

                asset->vram_size = texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings);
                asset->ram_size = asset->vram_size + sizeof(Texture);

                #if defined(OPENGL) || defined(VULKAN)
                    // If opengl, we always flip
                    if (!(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)) {
                        image_flip_vertical(ring, &texture->image);
                    }
                #endif
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
                UIThemeStyle* const theme = (UIThemeStyle *) asset->self;
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
        "[INFO] Loaded asset %d from archive %d for AMS %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_UINT64, &id}, {DATA_TYPE_UINT32, &element->type}, {DATA_TYPE_UINT8, &component_id}, {DATA_TYPE_UINT32, &element->length}, {DATA_TYPE_UINT32, &element->uncompressed}
    );

    // @performance maybe do in worker threads? This just feels very slow
    // @bug dependencies might be stored in different archives?
    for (uint32 i = 0; i < element->dependency_count; ++i) {
        asset_archive_asset_load(archive, id, ams, ring);
    }

    return asset;
}

#endif