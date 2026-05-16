/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
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
#include "AppCommand.h"

inline
UILayout* cmd_layout_load_sync(
    BufferMemory* const __restrict mem,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path
) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_LAYOUT_LOAD_SYNC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load layout");

    FileBody layout_file = {0};
    BUFFER_STACK_MEMORY_START(mem);
    file_read(layout_path, &layout_file, mem);

    if (!layout_file.content) {
        LOG_1("[WARNING] Failed loading layout");
        return NULL;
    }

    layout_from_data(layout_file.content, layout);

    return layout;
}

// @question Why does this not need the screen dimension but layout does
inline
UIThemeStyle* cmd_theme_load_sync(
    BufferMemory* const __restrict mem,
    UIThemeStyle* const __restrict theme, const wchar_t* const __restrict theme_path
) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_THEME_LOAD_SYNC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load theme");

    FileBody theme_file = {0};
    BUFFER_STACK_MEMORY_START(mem);
    file_read(theme_path, &theme_file, mem);
    theme_from_data(theme_file.content, theme);

    return theme;
}

FORCE_INLINE
void cmd_layout_populate_sync(
    UILayout* layout, const UIThemeStyle* theme
) NO_EXCEPT
{
    layout_from_theme(layout, theme);
}

// @question Do we really want the camera here or is a v2_uint16 sufficient?
inline
UILayout* cmd_ui_load_sync(
    BufferMemory* const __restrict mem,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path,
    UIThemeStyle* const __restrict general_theme,
    UIThemeStyle* const __restrict theme, const wchar_t* const __restrict theme_path,
    const Camera* const __restrict camera
) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_UI_LOAD_SYNC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load ui");

    if (!cmd_layout_load_sync(mem, layout, layout_path)) {
        // We have to make sure that at least the font is set
        layout->font = general_theme->font;

        return NULL;
    }

    cmd_layout_populate_sync(layout, general_theme);
    cmd_theme_load_sync(mem, theme, theme_path);
    cmd_layout_populate_sync(layout, theme);

    UIElement* const root = layout_get_element(layout, "root");
    UIWindow* const default_style = (UIWindow *) layout_get_element_style(layout, root, UI_STYLE_TYPE_DEFAULT);
    if (default_style) {
        default_style->dimension.dimension.width = camera->viewport_width;
        default_style->dimension.dimension.height = camera->viewport_height;
    }

    return layout;
}

// @question Do we really want the camera here or is a v2_uint16 sufficient?
static inline
UILayout* cmd_ui_load(
    ChunkMemory* const __restrict mem,
    Camera* const __restrict camera,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    // @performance I don't like using ChunkMemory here, we only need like 8 MB or even less.
    //              We should have a BufferMemory for stuff like this
    byte* temp;
    THRD_CHUNK_STACK_MEMORY(mem, &temp, 16 * MEGABYTE);

    BufferMemory buf;
    buffer_init(&buf, temp, 16 * MEGABYTE, 8);

    return cmd_ui_load_sync(
        &buf,
        &cmd->layout_body.scene_info->ui_layout, cmd->layout_body.layout_path,
        cmd->layout_body.general_theme,
        &cmd->layout_body.scene_info->ui_theme, cmd->layout_body.theme_path,
        camera
    );
}

#endif