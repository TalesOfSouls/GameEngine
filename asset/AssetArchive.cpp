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
#include "../object/Model.h"
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
#include "../gpuapi/GpuApiType.h"

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
        case ASSET_TYPE_MODEL:
            return sizeof(ModelFile);
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
        case ASSET_TYPE_MODEL:
            return alignof(ModelFile);
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
        case ASSET_TYPE_MODEL:
            return 64; // ModelFile::data must be MODEL_MEMORY_ALIGNMENT aligned, 64 for SIMD on the vertices
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
    if (archive->fd) {
        file_close_handle(archive->fd);
        archive->fd = 0;
    }

    if (archive->fd_async) {
        file_close_handle(archive->fd_async);
        archive->fd_async = 0;
    }

    // Not created in debug/internal builds because we need asset reloading for those
    if (archive->mmf) {
        file_mmf_close(archive->mmf);
        archive->mmf = 0;
    }
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

    // Hot reload
    str_copy(archive->path, path, (int32) OMS_MIN(str_length(path), (size_t) (PATH_MAX_LENGTH - 1)));
    archive->last_modified = file_last_modified(path);

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        // The archive can be rebuilt while the program runs (hot reload)
        // -> shared handles and no file mapping, a mapping prevents writing the file
        archive->fd = file_read_handle_shared(path);
    #else
        archive->fd = file_read_handle(path);
    #endif

    if (!archive->fd) {
        ASSERT_THROW();
        return;
    }

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        archive->fd_async = file_read_async_handle_shared(path);
    #else
        archive->fd_async = file_read_async_handle(path);
    #endif

    if (!archive->fd_async) {
        ASSERT_THROW();
        return;
    }

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        archive->mmf = 0;
    #else
        archive->mmf = file_mmf_handle(archive->fd_async);
    #endif

    FileBody file = {};

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

// Max. dependencies whose reads are started before the asset itself is decoded (same as Asset::references)
#define ASSET_ARCHIVE_PREFETCH_MAX MEMBER_ARRAY_COUNT(Asset, references)

// State of an asset load between asset_archive_load_begin() and asset_archive_load_finish()
struct AssetArchiveLoad {
    const AssetArchiveElement* element;
    int32 id;
    char id_str[9];

    // Existing asset (already_loaded = true) or the reserved asset
    Asset* asset;
    bool already_loaded;

    // Overlapped read into temporary memory
    FileBodyAsync file;
    byte* file_memory;
    int32 temp_element;
    uint32 temp_count;
};

/**
 * Starts loading an asset: starts the overlapped read and reserves the asset memory while the read is in flight
 *
 * @return bool false on failure
 */
static
bool asset_archive_load_begin(
    const AssetArchive* const archive,
    int32 id,
    AssetManagementSystem* const ams,
    ThrdChunkMemory* const mem,
    AssetArchiveLoad* const load
) NO_EXCEPT
{
    memset(load, 0, sizeof(AssetArchiveLoad));
    load->id = id;
    load->temp_element = -1;

    // Create a string representation from the asset id
    // We can't just use the asset id, since an int can have a \0 between high byte and low byte
    // @question We maybe can switch the AMS to work with ints as keys.
    //          We would then have to also create an application specific enum for general assets,
    //          that are not stored in the asset archive (e.g. color palette, which is generated at runtime).
    int_to_hex(id, load->id_str);

    if (ams) {
        // Check if asset already exists
        Asset* const asset = ams_asset_get_wait(ams, load->id_str);
        if (asset) {
            // Prevent garbage collection
            asset->state &= ~ASSET_MEMORY_STATE_RAM_GC;
            asset->state &= ~ASSET_MEMORY_STATE_VRAM_GC;

            load->asset = asset;
            load->already_loaded = true;

            return true;
        }
    }

    const AssetArchiveElement* const element = &archive->header.asset_element[ASSET_RAW_ID_FROM_ID(id)];
    load->element = element;

    ASSERT_TRUE(element->type < ASSET_TYPE_SIZE);
    ASSERT_TRUE(element->uncompressed > 0);

    LOG_2(
        "[INFO] Load asset %d from archive %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_INT32, &load->id},
        {DATA_TYPE_UINT32, (void *) &element->type},
        {DATA_TYPE_UINT32, (void *) &element->length},
        {DATA_TYPE_UINT32, (void *) &element->uncompressed}
    );

    // We are reading into temp memory since we have to perform transformations/decompression on the data
    // (released in asset_archive_load_finish(), multiple loads can be in flight at the same time)
    load->temp_count = (uint32) chunk_element_count(mem, element->length + 1);
    load->temp_element = chunk_reserve(mem, load->temp_count);
    if (load->temp_element < 0) {
        ASSERT_TRUE(false);
        return false;
    }

    load->file_memory = chunk_element_get(mem, load->temp_element);
    load->file.content = load->file_memory;
    file_read_async(archive->fd_async, &load->file, element->start, element->length);

    const size_t asset_memory_requirement = element->uncompressed // This is the data that gets stored in our specialized asset struct (e.g. .pixels in Image)
        + asset_type_size(element->type) // This is the specialized asset struct (e.g. Image, TextureAtlas, ...)
        + asset_align_size(element->type) // This is how we need to align our asset struct
        + asset_data_align_size(element->type); // This is how we need to align our data in the asset struct (e.g. .pixels in Image)

    // This happens while the file system loads the data
    // The important part is to reserve the uncompressed file size, not the compressed one
    // @bug Don't I have to mark_completed the underlying thread_chunk element? see code of function
    Asset* asset;
    if (ams) {
        asset = ams_asset_reserve(
            ams,
            load->id_str,
            (uint32) asset_memory_requirement
        );
        asset->official_id = id;
    } else {
        asset = (Asset *) memory_get(mem, sizeof(Asset) + ASSUMED_CACHE_LINE_SIZE + asset_memory_requirement);
        memset(asset, 0, sizeof(Asset) + ASSUMED_CACHE_LINE_SIZE + asset_memory_requirement);
        asset->data_size = (int32) asset_memory_requirement;
        asset->ram_size = (int32) (sizeof(Asset) + ASSUMED_CACHE_LINE_SIZE + asset_memory_requirement);
        asset->self = (byte *) ALIGN_UP((uintptr_t) (asset + 1), ASSUMED_CACHE_LINE_SIZE);
        asset->chunk_count = (uint16) chunk_element_count(
            mem,
            sizeof(Asset) + ASSUMED_CACHE_LINE_SIZE + asset_memory_requirement
        );
    }

    ASSERT_TRUE(asset_memory_requirement <= (size_t) asset->ram_size);

    asset->data_size = element->uncompressed;
    asset->state |= ASSET_MEMORY_STATE_IN_RAM;

    load->asset = asset;

    return true;
}

/**
 * Waits for the read of asset_archive_load_begin(), decodes the asset and releases the temporary memory
 */
static
void asset_archive_load_finish(
    const AssetArchive* const archive,
    AssetManagementSystem* const ams,
    ThrdChunkMemory* const mem,
    AssetArchiveLoad* const load
) NO_EXCEPT
{
    if (load->already_loaded || !load->asset) {
        return;
    }

    // Sets file.size to the bytes actually read
    if (!file_async_wait(archive->fd_async, &load->file, true) || load->file.size != load->element->length) {
        LOG_1(
            "[ERROR] Couldn't read asset %d from archive (%n of %n B)",
            {DATA_TYPE_INT32, &load->id},
            {DATA_TYPE_UINT64, &load->file.size},
            {DATA_TYPE_UINT32, (void *) &load->element->length}
        );
        ASSERT_TRUE(false);
    }

    // @bug Couldn't the asset become available from ams_asset_get_wait to here?
    // This would mean we are overwriting it
    // A solution could be a function called ams_get_reserve_wait() that reserves, if not available
    // However, that function would have to lock the ams during that entire time
    switch (load->element->type) {
        case ASSET_TYPE_GENERAL: {
            byte* const raw_data = load->asset->self;

            lz4_decode(load->file.content, load->file.size, raw_data);
        } break;
        case ASSET_TYPE_TEXTURE_ATLAS: {
            TextureAtlas* const atlas = (TextureAtlas *) load->asset->self;
            atlas->elements = (TextureAtlasElement *) (atlas + 1);

            atlas_from_data(load->file.content, atlas);
        } break;
        case ASSET_TYPE_IMAGE: {
            Texture* texture = (Texture *) load->asset->self;
            texture->image.pixels = (byte *) ALIGN_UP((uintptr_t) (texture + 1), 64);

            load->file.content += image_header_from_data(load->file.content, &texture->image);
            qoi_decode(load->file.content, &texture->image);

            load->asset->vram_size = texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings);
        } break;
        case ASSET_TYPE_AUDIO: {
            Audio* const audio = (Audio *) load->asset->self;
            audio->data = (byte *) (audio + 1);

            const uint32 header_size = audio_header_from_data(load->file.content, audio);
            load->file.content += header_size;

            const uint32 decoded = qoa_decode(load->file.content, audio, (uint32) (load->element->length - header_size));
            // Never play uninitialized memory if the asset is shorter than its header claims
            if (decoded < audio->size) {
                LOG_1("[WARNING] Audio asset %d only decoded %n of %n B", {DATA_TYPE_INT32, &load->id}, {DATA_TYPE_UINT32, (void *) &decoded}, {DATA_TYPE_UINT32, &audio->size});
                audio->size = decoded;
            }
        } break;
        case ASSET_TYPE_OBJ: {
            Mesh* const mesh = (Mesh *) load->asset->self;
            mesh->data = (byte *) (mesh + 1);

            mesh_from_data(load->file.content, mesh);
        } break;
        case ASSET_TYPE_LANGUAGE: {
            Language* const language = (Language *) load->asset->self;
            language->data = (byte *) (language + 1);

            language_from_data(load->file.content, language);
        } break;
        case ASSET_TYPE_FONT: {
            Font* const font = (Font *) load->asset->self;
            font->glyphs = (Glyph *) (font + 1);

            font_from_data(load->file.content, font);
        } break;
        case ASSET_TYPE_THEME: {
            UITheme* const theme = (UITheme *) load->asset->self;
            theme->data = (byte *) (theme + 1);

            theme_from_data(load->file.content, theme);
        } break;
        case ASSET_TYPE_MODEL: {
            ModelFile* const model = (ModelFile *) load->asset->self;
            model->data = (byte *) ALIGN_UP((uintptr_t) (model + 1), 64);

            if (!model_from_data(load->file.content, load->file.size, model)) {
                LOG_1("[ERROR] Model asset %d is invalid", {DATA_TYPE_INT32, &load->id});
                ASSERT_TRUE(false);
            }
        } break;
        default: {
            UNREACHABLE();
        }
    }

    // The temporary read memory is no longer needed
    if (load->temp_element >= 0) {
        chunk_free_elements(mem, load->temp_element, load->temp_count);
        load->temp_element = -1;
    }

    // Even though dependencies are still being loaded
    // the main program should still be able to do some work if possible
    if (ams) {
        ams_set_loaded(ams, load->asset);
    }

    LOG_2(
        "[INFO] Loaded asset %d from archive %d with %n B compressed and %n B uncompressed",
        {DATA_TYPE_INT32, &load->id},
        {DATA_TYPE_UINT32, (void *) &load->element->type},
        {DATA_TYPE_UINT32, (void *) &load->element->length},
        {DATA_TYPE_UINT32, (void *) &load->element->uncompressed}
    );

    if (load->element->dependency_count) {
        // Asset::references only has room for ASSET_ARCHIVE_PREFETCH_MAX dependencies
        const uint32 count = OMS_MIN(load->element->dependency_count, (uint32) ASSET_ARCHIVE_PREFETCH_MAX);
        ASSERT_TRUE(load->element->dependency_count <= ASSET_ARCHIVE_PREFETCH_MAX);

        load->asset->reference_count = (uint16) count;
        memcpy(
            load->asset->references,
            &archive->header.asset_dependencies[load->element->dependency_start],
            sizeof(uint32) * count
        );
    }
}

/**
 * Loads an asset (and its dependencies)
 *
 * The reads of the direct dependencies are started BEFORE the asset itself is decoded,
 * this way the file system works on the dependencies while we wait for/decode the asset.
 * Deeper dependencies are handled recursively (with the same prefetching).
 */
// @bug I'm afraid that loading the same asset twice could result in circumstances where it gets added twice
Asset* asset_archive_asset_load(
    const AssetArchive* const archive,
    int32 id,
    AssetManagementSystem* const ams,
    ThrdChunkMemory* const mem,
    bool load_dependencies = true
) NO_EXCEPT
{
    char id_str[9];
    int_to_hex(id, id_str);
    PROFILE_DEBUG(PROFILE_ASSET_ARCHIVE_ASSET_LOAD, id_str, PROFILE_FLAG_SHOULD_LOG);

    AssetArchiveLoad load;
    if (!asset_archive_load_begin(archive, id, ams, mem, &load)) {
        return NULL;
    }

    if (load.already_loaded) {
        return load.asset;
    }

    // @question how to handle references if we use this in a temp way = without AMS
    const bool with_dependencies = load_dependencies && ams && load.element->dependency_count;

    // 1. Start the reads of the direct dependencies (the reads run while we decode this asset)
    AssetArchiveLoad deps[ASSET_ARCHIVE_PREFETCH_MAX];
    uint32 dep_count = 0;

    if (with_dependencies) {
        const uint32 count = OMS_MIN(load.element->dependency_count, (uint32) ASSET_ARCHIVE_PREFETCH_MAX);
        const uint32* const dep_ids = &archive->header.asset_dependencies[load.element->dependency_start];

        for (uint32 i = 0; i < count; ++i) {
            if (asset_archive_load_begin(archive, (int32) dep_ids[i], ams, mem, &deps[dep_count])) {
                ++dep_count;
            }
        }
    }

    // 2. Decode this asset
    asset_archive_load_finish(archive, ams, mem, &load);

    // 3. Decode the dependencies (their reads are most likely already done) and load their dependencies
    for (uint32 i = 0; i < dep_count; ++i) {
        const bool was_loaded = deps[i].already_loaded;
        asset_archive_load_finish(archive, ams, mem, &deps[i]);

        if (!was_loaded && deps[i].element && deps[i].element->dependency_count) {
            const uint32 count = OMS_MIN(deps[i].element->dependency_count, (uint32) ASSET_ARCHIVE_PREFETCH_MAX);
            for (uint32 j = 0; j < count; ++j) {
                asset_archive_asset_load(archive, (int32) deps[i].asset->references[j], ams, mem);
            }
        }
    }

    return load.asset;
}

/**
 * Hot reload of asset archives
 *
 * A rebuilt archive replaces the header of the loaded archive and the assets that are
 * already loaded are decoded again into their existing memory. Everything that references
 * the asset memory stays valid (e.g. font->glyphs, the texture of an atlas).
 *
 * Restrictions (-> restart required)
 *      - the amount of assets/dependencies and the type of every asset must stay the same
 *      - a reloaded asset must fit into the memory of the loaded version
 *
 * The gpu data is not updated, assets with gpu data are marked with ASSET_MEMORY_STATE_VRAM_OUTDATED
 */
enum AssetArchiveReloadResult : int32 {
    ASSET_ARCHIVE_RELOAD_OK,

    // The file couldn't be opened or read (e.g. the build is still running)
    ASSET_ARCHIVE_RELOAD_FAILED,

    // The structure of the archive changed or the asset doesn't fit into its memory -> restart required
    ASSET_ARCHIVE_RELOAD_CHANGED,
};

/**
 * Checks if the archive file changed since it was loaded
 *
 * @param const AssetArchive* archive Archive
 *
 * @return bool
 */
inline
bool asset_archive_has_changed(const AssetArchive* const archive) NO_EXCEPT
{
    return archive->path[0] && file_last_modified(archive->path) != archive->last_modified;
}

/**
 * Loads the header of the rebuilt archive file and opens the file again
 *
 * All or nothing: if anything fails the archive stays as it is.
 * The assets themselves are not reloaded
 *
 * @param AssetArchive* archive Archive
 * @param BufferMemory* mem     Temporary memory
 * @param int32         steps   Simd steps
 *
 * @return AssetArchiveReloadResult
 */
AssetArchiveReloadResult asset_archive_reload(
    AssetArchive* const archive,
    BufferMemory* const mem,
    int32 steps = 8
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_ASSET_ARCHIVE_LOAD, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);

    // version + asset_count + asset_dependency_count
    CONSTEXPR uint32 HEADER_BASE_SIZE = sizeof(int32) + sizeof(uint32) + sizeof(uint32);

    const uint64 last_modified = file_last_modified(archive->path);
    const size_t archive_size = file_size(archive->path);
    if (archive_size < HEADER_BASE_SIZE) {
        LOG_1("[WARNING] Asset reload: couldn't read %s", {DATA_TYPE_WCHAR_STR, (void *) archive->path});

        return ASSET_ARCHIVE_RELOAD_FAILED;
    }

    #if _WIN32
        FileHandle fd = file_read_handle_shared(archive->path);
        FileHandle fd_async = file_read_async_handle_shared(archive->path);
    #else
        FileHandle fd = file_read_handle(archive->path);
        FileHandle fd_async = file_read_async_handle(archive->path);
    #endif

    AssetArchiveReloadResult result = ASSET_ARCHIVE_RELOAD_FAILED;

    BUFFER_STACK_MEMORY_START(mem);

    // The archive keeps reading from the old handles until everything was checked
    do {
        if (!fd || !fd_async) {
            break;
        }

        FileBody file = {};
        file.content = memory_get(mem, HEADER_BASE_SIZE + 1, alignof(size_t));
        file_read(fd, &file, 0, HEADER_BASE_SIZE);
        if (!file.content || file.size != HEADER_BASE_SIZE) {
            break;
        }

        int32 version;
        uint32 asset_count;
        uint32 asset_dependency_count;
        const byte* pos = file.content;
        pos = read_le(pos, &version);
        pos = read_le(pos, &asset_count);
        read_le(pos, &asset_dependency_count);

        if (version != ASSET_ARCHIVE_VERSION
            || asset_count != archive->header.asset_count
            || asset_dependency_count != archive->header.asset_dependency_count
        ) {
            LOG_1(
                "[WARNING] Asset reload: the structure of %s changed from %d to %d assets (restart required)",
                {DATA_TYPE_WCHAR_STR, (void *) archive->path},
                {DATA_TYPE_UINT32, (void *) &archive->header.asset_count},
                {DATA_TYPE_UINT32, (void *) &asset_count}
            );

            result = ASSET_ARCHIVE_RELOAD_CHANGED;
            break;
        }

        const size_t table_size = asset_count * sizeof(AssetArchiveElement)
            + asset_dependency_count * sizeof(uint32);
        const size_t header_size = HEADER_BASE_SIZE + table_size;

        if (header_size > archive_size || table_size > archive->data_size) {
            break;
        }

        file.content = memory_get(mem, header_size + 1, alignof(size_t));
        file_read(fd, &file, 0, header_size);
        if (!file.content || file.size != header_size) {
            break;
        }

        // Parse into temporary memory first, the loaded header stays untouched until everything was checked
        AssetArchiveHeader next = {};
        next.asset_element = (AssetArchiveElement *) memory_get(mem, archive->data_size, ASSUMED_CACHE_LINE_SIZE);
        asset_archive_header_load(&next, archive->data_size, file.content, steps);

        bool is_valid = true;
        for (uint32 i = 0; i < asset_count; ++i) {
            const AssetArchiveElement* const element = &next.asset_element[i];

            if (element->type != archive->header.asset_element[i].type) {
                LOG_1(
                    "[WARNING] Asset reload: the type of asset %d in %s changed (restart required)",
                    {DATA_TYPE_UINT32, (void *) &i},
                    {DATA_TYPE_WCHAR_STR, (void *) archive->path}
                );

                result = ASSET_ARCHIVE_RELOAD_CHANGED;
                is_valid = false;
                break;
            }

            // e.g. the file is still being written
            if ((uint64) element->start + element->length > archive_size
                || element->dependency_start + element->dependency_count > asset_dependency_count
            ) {
                is_valid = false;
                break;
            }
        }

        if (!is_valid) {
            break;
        }

        // Replace the header
        memcpy(archive->data, next.asset_element, table_size);
        archive->header.version = next.version;
        archive->header.asset_element = (AssetArchiveElement *) archive->data;
        archive->header.asset_dependencies = asset_dependency_count
            ? (uint32 *) (archive->data + asset_count * sizeof(AssetArchiveElement))
            : NULL;

        // Replace the file handles (the file may have been replaced by a new file)
        asset_archive_close(archive);
        archive->fd = fd;
        archive->fd_async = fd_async;
        fd = 0;
        fd_async = 0;

        archive->last_modified = last_modified;
        result = ASSET_ARCHIVE_RELOAD_OK;
    } while (false);

    if (fd) {
        file_close_handle(fd);
    }

    if (fd_async) {
        file_close_handle(fd_async);
    }

    LOG_TRUE_1(
        result == ASSET_ARCHIVE_RELOAD_OK,
        "[INFO] Asset reload: reloaded the header of %s",
        {DATA_TYPE_WCHAR_STR, (void *) archive->path}
    );
    LOG_TRUE_1(
        result == ASSET_ARCHIVE_RELOAD_FAILED,
        "[WARNING] Asset reload: couldn't read %s",
        {DATA_TYPE_WCHAR_STR, (void *) archive->path}
    );

    return result;
}

/**
 * Reloads an asset that is already loaded (asset management system) from its archive
 *
 * The asset is decoded again into its existing memory. The runtime data of the asset is kept:
 *      Texture:        gpu data (texture id, sampler, ...), the vertical orientation is the same as before
 *      Mesh:           gpu data (vao, vbo, ...)
 *      Font:           texture, the coordinates are inverted like cmd_font_load_sync() does
 *      TextureAtlas:   texture, the coordinates are inverted like cmd_texture_atlas_load() does
 * Assets with gpu data in vram are marked with ASSET_MEMORY_STATE_VRAM_OUTDATED.
 *
 * WARNING: The header of the archive must be reloaded before
 *
 * @param const AssetArchive* archive      Archive of the asset
 * @param Asset*              asset        Loaded asset (official_id is the asset id)
 * @param ThrdChunkMemory*    mem          Temporary memory for the file data
 * @param GpuApiType          gpu_api_type Gpu api (orientation of the textures)
 *
 * @return AssetArchiveReloadResult
 */
AssetArchiveReloadResult asset_archive_asset_reload(
    const AssetArchive* const archive,
    Asset* const asset,
    ThrdChunkMemory* const mem,
    GpuApiType gpu_api_type
) NO_EXCEPT
{
    const uint32 raw_id = ASSET_RAW_ID_FROM_ID(asset->official_id);
    if (raw_id >= archive->header.asset_count || !asset->self) {
        return ASSET_ARCHIVE_RELOAD_FAILED;
    }

    const AssetArchiveElement* const element = &archive->header.asset_element[raw_id];
    ASSERT_TRUE(element->type < ASSET_TYPE_SIZE);

    // Same requirement as asset_archive_load_begin() (images are checked exactly below)
    const size_t asset_memory_requirement = element->uncompressed
        + asset_type_size(element->type)
        + asset_align_size(element->type)
        + asset_data_align_size(element->type);

    if (element->uncompressed == 0 || asset_memory_requirement > (size_t) asset->ram_size) {
        LOG_1(
            "[WARNING] Asset reload: asset %d grew to %n B, only %n B are reserved (restart required)",
            {DATA_TYPE_UINT32, (void *) &asset->official_id},
            {DATA_TYPE_UINT64, (void *) &asset_memory_requirement},
            {DATA_TYPE_INT32, (void *) &asset->ram_size}
        );

        return ASSET_ARCHIVE_RELOAD_CHANGED;
    }

    // Read into temporary memory
    const uint32 temp_count = (uint32) chunk_element_count(mem, element->length + 1);
    const int32 temp_element = chunk_reserve(mem, temp_count);
    if (temp_element < 0) {
        return ASSET_ARCHIVE_RELOAD_FAILED;
    }

    FileBodyAsync file = {};
    file.content = chunk_element_get(mem, temp_element);
    file_read_async(archive->fd_async, &file, element->start, element->length);

    if (!file_async_wait(archive->fd_async, &file, true) || file.size != element->length) {
        chunk_free_elements(mem, temp_element, temp_count);

        return ASSET_ARCHIVE_RELOAD_FAILED;
    }

    // Same gpu apis as cmd_texture_load_sync(), cmd_font_load_sync(), cmd_texture_atlas_load()
    const bool invert_coordinates = gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE;

    switch (element->type) {
        case ASSET_TYPE_GENERAL: {
            lz4_decode(file.content, file.size, asset->self);
        } break;
        case ASSET_TYPE_TEXTURE_ATLAS: {
            TextureAtlas* const atlas = (TextureAtlas *) asset->self;
            Texture* const texture = atlas->texture;

            atlas->elements = (TextureAtlasElement *) (atlas + 1);
            atlas_from_data(file.content, atlas);
            atlas->texture = texture;

            if (invert_coordinates) {
                atlas_invert_coordinates(atlas);
            }
        } break;
        case ASSET_TYPE_IMAGE: {
            Texture* const texture = (Texture *) asset->self;

            // Gpu data (everything in front of the image)
            byte gpu_data[offsetof(Texture, image)];
            memcpy(gpu_data, texture, sizeof(gpu_data));

            // The texture keeps its orientation
            const byte orientation = texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP;

            // Images are decoded into a Texture (> Image) -> exact check of the pixel memory
            Image header = {};
            const byte* data = file.content;
            data += image_header_from_data(data, &header);

            byte* const pixels = (byte *) ALIGN_UP((uintptr_t) (texture + 1), 64);
            const uint64 pixel_size = (uint64) header.pixel_count * image_pixel_size_from_type(header.image_settings);
            if ((uintptr_t) pixels + pixel_size > (uintptr_t) asset->self + (uint32) asset->ram_size) {
                chunk_free_elements(mem, temp_element, temp_count);
                LOG_1(
                    "[WARNING] Asset reload: image %d grew to %n B (restart required)",
                    {DATA_TYPE_UINT32, (void *) &asset->official_id},
                    {DATA_TYPE_UINT64, (void *) &pixel_size}
                );

                return ASSET_ARCHIVE_RELOAD_CHANGED;
            }

            header.pixels = pixels;
            texture->image = header;
            qoi_decode(data, &texture->image);

            if ((texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP) != orientation) {
                image_flip_vertical(&texture->image);
            }

            memcpy(texture, gpu_data, sizeof(gpu_data));

            asset->vram_size = texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings);
        } break;
        case ASSET_TYPE_AUDIO: {
            Audio* const audio = (Audio *) asset->self;
            audio->data = (byte *) (audio + 1);

            const uint32 header_size = audio_header_from_data(file.content, audio);
            const uint32 decoded = qoa_decode(file.content + header_size, audio, (uint32) (element->length - header_size));

            // Never play uninitialized memory if the asset is shorter than its header claims
            if (decoded < audio->size) {
                audio->size = decoded;
            }
        } break;
        case ASSET_TYPE_OBJ: {
            Mesh* const mesh = (Mesh *) asset->self;

            // Gpu data (union of the gpu apis)
            CONSTEXPR size_t GPU_DATA_OFFSET = offsetof(Mesh, opengl_mesh);
            CONSTEXPR size_t GPU_DATA_SIZE = offsetof(Mesh, material_count) - GPU_DATA_OFFSET;
            byte gpu_data[GPU_DATA_SIZE];
            memcpy(gpu_data, (byte *) mesh + GPU_DATA_OFFSET, GPU_DATA_SIZE);

            mesh->data = (byte *) (mesh + 1);
            mesh_from_data(file.content, mesh);

            memcpy((byte *) mesh + GPU_DATA_OFFSET, gpu_data, GPU_DATA_SIZE);
        } break;
        case ASSET_TYPE_LANGUAGE: {
            Language* const language = (Language *) asset->self;
            language->data = (byte *) (language + 1);

            language_from_data(file.content, language);
        } break;
        case ASSET_TYPE_FONT: {
            Font* const font = (Font *) asset->self;
            Texture* const texture = font->texture;

            font->glyphs = (Glyph *) (font + 1);
            font_from_data(file.content, font);
            font->texture = texture;

            if (invert_coordinates) {
                font_invert_coordinates(font);
            }
        } break;
        case ASSET_TYPE_THEME: {
            UITheme* const theme = (UITheme *) asset->self;
            theme->data = (byte *) (theme + 1);

            theme_from_data(file.content, theme);
        } break;
        case ASSET_TYPE_MODEL: {
            ModelFile* const model = (ModelFile *) asset->self;
            byte* const memory = (byte *) ALIGN_UP((uintptr_t) (model + 1), 64);

            // Exact check of the data block (the binary header is not part of the memory)
            const uint64 block_size = model_data_block_size(file.content, file.size);
            if (!block_size || (uintptr_t) memory + block_size > (uintptr_t) asset->self + (uint32) asset->ram_size) {
                chunk_free_elements(mem, temp_element, temp_count);

                return block_size ? ASSET_ARCHIVE_RELOAD_CHANGED : ASSET_ARCHIVE_RELOAD_FAILED;
            }

            model->data = memory;
            model_from_data(file.content, file.size, model);
        } break;
        default: {
            UNREACHABLE();
        }
    }

    chunk_free_elements(mem, temp_element, temp_count);

    asset->data_size = element->uncompressed;

    if (element->dependency_count) {
        const uint32 count = OMS_MIN(element->dependency_count, (uint32) ASSET_ARCHIVE_PREFETCH_MAX);
        asset->reference_count = (uint16) count;
        memcpy(
            asset->references,
            &archive->header.asset_dependencies[element->dependency_start],
            sizeof(uint32) * count
        );
    }

    if ((element->type == ASSET_TYPE_IMAGE || element->type == ASSET_TYPE_OBJ || element->type == ASSET_TYPE_MODEL)
        && (asset->state & ASSET_MEMORY_STATE_IN_VRAM)
    ) {
        asset->state |= ASSET_MEMORY_STATE_VRAM_OUTDATED;
    }

    LOG_2("[INFO] Asset reload: reloaded asset %d", {DATA_TYPE_UINT32, (void *) &asset->official_id});

    return ASSET_ARCHIVE_RELOAD_OK;
}

#endif
