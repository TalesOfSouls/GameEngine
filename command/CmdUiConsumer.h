/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_UI_CONSUMER_H
#define COMS_APP_COMMAND_UI_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.cpp"
#include "../asset/Asset.h"
#include "../asset/AssetArchive.cpp"
#include "../asset/AssetManagementSystem.cpp"
#include "../gpuapi/GpuApiType.h"
#include "../system/FileUtils.cpp"
#include "../ui/UILayout.cpp"
#include "../ui/UITheme.cpp"
#include "../font/FontSystem.cpp"
#include "AppCommand.h"
#include "AppCmdBuffer.h"
#include "CmdFontConsumer.h"
#include "CmdTextureConsumer.h"

/**
 * Loads a binary layout
 */
inline
UILayout* cmd_layout_load_sync(
    BufferMemory* const __restrict mem,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CMD_LAYOUT_LOAD_SYNC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load layout");

    FileBody layout_file = {0};
    BUFFER_STACK_MEMORY_START(mem);
    file_read(layout_path, &layout_file, mem);

    if (!layout_file.content) {
        LOG_1("[WARNING] Failed loading layout");
        return NULL;
    }

    if (layout_from_data(layout_file.content, layout) < 0) {
        // e.g. layout file was created with an older layout version -> needs to be rebuilt
        LOG_1("[WARNING] Incompatible layout file");
        return NULL;
    }

    return layout;
}

/**
 * Helper struct for loading the theme
 */
struct CmdThemeAssets {
    const AssetArchive* archives;
    AssetManagementSystem* ams;
    ThrdChunkMemory* mem;
    GpuApiType gpu_api_type;

    // A theme may reference another theme (e.g. general theme), but it is optional
    const UITheme* shared;

    // FontSystem of the theme
    FontSystem* font_storage;
};

/**
 * Loads the assets of a theme ("@theme" block: font, atlas) or re-uses them:
 *      1. same asset id as the shared theme (e.g. general theme) -> the font system/atlas of the shared theme is used
 *      2. asset already loaded (asset management system) -> the asset is used as is
 *      3. otherwise the asset is loaded from its archive (incl. the texture)
 * An asset that is not defined by the theme stays NULL (-> the ui uses the font/atlas of another theme)
 *
 * @return false if a defined asset couldn't be loaded
 */
inline
bool cmd_theme_assets_load_sync(
    const CmdThemeAssets* const __restrict assets,
    UITheme* const __restrict theme
) NO_EXCEPT
{
    bool success = true;

    theme->font = NULL;
    theme->atlas = NULL;

    // Font
    if (theme->font_asset_id != UI_THEME_ASSET_NONE) {
        if (assets->shared && assets->shared->font
            && assets->shared->font_asset_id == theme->font_asset_id
        ) {
            theme->font = assets->shared->font;
        } else if (theme->font_asset_id) {
            const Asset* const font_asset = cmd_font_load_sync(
                assets->archives, assets->ams, assets->mem, assets->gpu_api_type,
                theme->font_asset_id
            );

            // The font system is a copy of the font + caches
            FontSystem* const font_system = assets->font_storage;
            memset(font_system, 0, sizeof(*font_system));
            memcpy(&font_system->base, (const Font *) font_asset->self, sizeof(Font));
            font_system_newline_cache(font_system);

            theme->font = font_system;
        } else {
            success = false;
        }
    }

    // Texture atlas for the images of the layouts
    if (theme->atlas_asset_id != UI_THEME_ASSET_NONE) {
        if (assets->shared && assets->shared->atlas
            && assets->shared->atlas_asset_id == theme->atlas_asset_id
        ) {
            theme->atlas = assets->shared->atlas;
        } else if (theme->atlas_asset_id) {
            const Asset* const atlas_asset = cmd_texture_atlas_load(
                assets->archives, assets->ams, assets->mem, assets->gpu_api_type,
                theme->atlas_asset_id
            );

            theme->atlas = (const TextureAtlas *) atlas_asset->self;
        } else {
            success = false;
        }
    }

    return success;
}

/**
 * Loads a theme incl. everything the theme references:
 *      language: a text theme loads its language file
 *      assets:   font + atlas of the "@theme" block, only if assets != NULL
 *
 * WARNING: theme->data and theme->data_size need to be set
 *
 * @param language_code e.g. L"en_US", NULL = LANGUAGE_DEFAULT_CODE
 * @param assets        Where the assets are loaded from, NULL = the assets are not loaded (e.g. tools)
 */
inline
UITheme* cmd_theme_load_sync(
    BufferMemory* const __restrict mem,
    UITheme* const __restrict theme, const wchar_t* const __restrict theme_path,
    const wchar_t* const __restrict language_code = NULL,
    const CmdThemeAssets* const __restrict assets = NULL
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CMD_THEME_LOAD_SYNC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load theme");

    FileBody theme_file = {0};
    BUFFER_STACK_MEMORY_START(mem);
    file_read(theme_path, &theme_file, mem);

    if (!theme_file.content) {
        LOG_1("[WARNING] Failed loading theme");

        return NULL;
    }

    // @todo remove once we switch to themebin
    if (str_ends_with(theme_path, L".themetxt")) {
        if (!ui_theme_from_txt(theme, (const char *) theme_file.content, (int32) theme_file.size)) {
            return NULL;
        }

        // Optional, the theme works without strings
        ui_theme_language_load(theme, theme_path, language_code, mem);
    } else if (theme_from_data(theme_file.content, theme) < 0) {
        return NULL;
    }

    if (assets && !cmd_theme_assets_load_sync(assets, theme)) {
        LOG_1("[ERROR] Couldn't load all assets of the theme");
    }

    return theme;
}

inline
UILayout* cmd_ui_load_sync(
    BufferMemory* const __restrict mem,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path,
    UITheme* const __restrict general_theme,
    UITheme* const __restrict theme, const wchar_t* const __restrict theme_path,
    const wchar_t* const __restrict language_code = NULL,
    const CmdThemeAssets* const __restrict assets = NULL
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CMD_UI_LOAD_SYNC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load ui");

    // We have to make sure that at least the font is set
    if (general_theme->font) {
        layout->font = general_theme->font;
    }

    // The styles are applied when the elements are created in ui_layout_from_txt()
    // That means the theme has to be loaded first
    // The assets of the general theme are re-used
    CmdThemeAssets theme_assets = {};
    if (assets) {
        theme_assets = *assets;
        theme_assets.shared = general_theme;
        theme_assets.font_storage = NULL;
    }

    cmd_theme_load_sync(mem, theme, theme_path, language_code, assets ? &theme_assets : NULL);

    if (theme->font) {
        layout->font = theme->font;
    }

    return cmd_layout_load_sync(mem, layout, layout_path);
}

static inline
UILayout* cmd_ui_load(
    AppCmdBuffer* const __restrict cb,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    BufferMemory buf;
    THRD_CHUNK_STACK_MEMORY(cb->mem, &buf, 16 * MEGABYTE);

    CmdThemeAssets assets = {};
    assets.archives = cb->asset_archives;
    assets.ams = cb->ams;
    assets.mem = cb->mem;
    assets.gpu_api_type = cb->gpu_api_type;

    // @question Do we really want to do the relative_to_absolute here,
    //          or closer to where it is used?
    wchar_t layout_path[PATH_MAX_LENGTH];
    relative_to_absolute(cmd->layout_body.layout_path, layout_path);

    wchar_t theme_path[PATH_MAX_LENGTH];
    relative_to_absolute(cmd->layout_body.theme_path, theme_path);

    return cmd_ui_load_sync(
        &buf,
        cmd->layout_body.layout, layout_path,
        cmd->layout_body.general_theme,
        cmd->layout_body.theme, theme_path,
        cmd->layout_body.language[0] ? cmd->layout_body.language : NULL,
        &assets
    );
}

#endif