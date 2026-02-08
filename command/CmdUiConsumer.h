/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_UI_CONSUMER_H
#define COMS_APP_COMMAND_UI_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.h"
#include "../asset/Asset.h"
#include "../asset/AssetArchive.h"
#include "../asset/AssetManagementSystem.h"
#include "../gpuapi/GpuApiType.h"
#include "../system/FileUtils.cpp"
#include "../ui/UILayout.cpp"
#include "../ui/UITheme.h"
#include "AppCommand.h"

inline
UILayout* cmd_layout_load_sync(
    RingMemory* const __restrict ring,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path
) NO_EXCEPT
{
    //PROFILE(PROFILE_CMD_LAYOUT_LOAD_SYNC, layout_path, PROFILE_FLAG_SHOULD_LOG);
    //LOG_1("Load layout %s", {DATA_TYPE_CHAR_STR, (void *) layout_path});

    FileBody layout_file = {0};
    file_read(layout_path, &layout_file, ring);

    if (!layout_file.content) {
        LOG_1("Failed loading layout");
        return NULL;
    }

    layout_from_data(layout_file.content, layout);

    return layout;
}

// @question Why does this not need the screen dimension but layout does
inline
UIThemeStyle* cmd_theme_load_sync(
    RingMemory* const __restrict ring,
    UIThemeStyle* const __restrict theme, const wchar_t* const __restrict theme_path
) NO_EXCEPT
{
    //PROFILE(PROFILE_CMD_THEME_LOAD_SYNC, theme_path, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Load theme");

    FileBody theme_file = {0};
    file_read(theme_path, &theme_file, ring);
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
    RingMemory* const __restrict ring,
    UILayout* const __restrict layout, const wchar_t* const __restrict layout_path,
    UIThemeStyle* const __restrict general_theme,
    UIThemeStyle* const __restrict theme, const wchar_t* const __restrict theme_path,
    const Camera* const __restrict camera
) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_UI_LOAD_SYNC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Load ui");

    if (!cmd_layout_load_sync(ring, layout, layout_path)) {
        // We have to make sure that at least the font is set
        layout->font = general_theme->font;

        return NULL;
    }

    cmd_layout_populate_sync(layout, general_theme);
    cmd_theme_load_sync(ring, theme, theme_path);
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
    RingMemory* const __restrict ring,
    Camera* const __restrict camera,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    return cmd_ui_load_sync(
        ring,
        cmd->layout_body.layout, cmd->layout_body.layout_path,
        cmd->layout_body.general_theme,
        cmd->layout_body.theme, cmd->layout_body.theme_path,
        camera
    );
}

#endif