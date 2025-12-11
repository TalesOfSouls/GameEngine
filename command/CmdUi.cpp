/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_UI_C
#define COMS_APP_COMMAND_UI_C

inline
UILayout* cmd_layout_load_sync(
    AppCmdBuffer* const __restrict cb,
    UILayout* const __restrict layout, const wchar_t* __restrict layout_path
) NO_EXCEPT
{
    //PROFILE(PROFILE_CMD_LAYOUT_LOAD_SYNC, layout_path, PROFILE_FLAG_SHOULD_LOG);
    //LOG_1("Load layout %s", {DATA_TYPE_CHAR_STR, (void *) layout_path});

    FileBody layout_file = {};
    file_read(layout_path, &layout_file, cb->mem_vol);

    if (!layout_file.content) {
        LOG_1("Failed loading layout");
        return NULL;
    }

    layout_from_data(layout_file.content, layout);

    return layout;
}

inline
UIThemeStyle* cmd_theme_load_sync(
    AppCmdBuffer* const __restrict cb,
    UIThemeStyle* __restrict theme, const wchar_t* __restrict theme_path
) NO_EXCEPT
{
    //PROFILE(PROFILE_CMD_THEME_LOAD_SYNC, theme_path, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Load theme");

    FileBody theme_file = {};
    file_read(theme_path, &theme_file, cb->mem_vol);
    theme_from_data(theme_file.content, theme);

    return theme;
}

FORCE_INLINE
void cmd_layout_populate_sync(
    AppCmdBuffer*,
    UILayout* layout, const UIThemeStyle* theme
) NO_EXCEPT
{
    layout_from_theme(layout, theme);
}

inline
UILayout* cmd_ui_load_sync(
    AppCmdBuffer* const __restrict cb,
    UILayout* const __restrict layout, const wchar_t* __restrict const layout_path,
    UIThemeStyle* __restrict const general_theme,
    UIThemeStyle* __restrict const theme, const wchar_t* __restrict const theme_path,
    const Camera* const __restrict camera
) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_UI_LOAD_SYNC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Load ui");

    if (!cmd_layout_load_sync(cb, layout, layout_path)) {
        // We have to make sure that at least the font is set
        layout->font = general_theme->font;

        return NULL;
    }

    cmd_layout_populate_sync(cb, layout, general_theme);
    cmd_theme_load_sync(cb, theme, theme_path);
    cmd_layout_populate_sync(cb, layout, theme);

    UIElement* root = layout_get_element(layout, "root");
    UIWindow* const default_style = (UIWindow *) layout_get_element_style(layout, root, UI_STYLE_TYPE_DEFAULT);
    if (default_style) {
        default_style->dimension.dimension.width = camera->viewport_width;
        default_style->dimension.dimension.height = camera->viewport_height;
    }

    return layout;
}

static inline
UILayout* cmd_ui_load(AppCmdBuffer* const __restrict cb, const Command* const __restrict cmd) NO_EXCEPT
{
    const byte* pos = cmd->data;

    SceneInfo* const scene = (SceneInfo *) *((uintptr_t *) pos);
    pos += sizeof(uintptr_t);

    const wchar_t* const layout_path = (wchar_t *) pos;
    str_move_to((const wchar_t **) &pos, L'\0');
    pos += sizeof(L'\0');

    UIThemeStyle* const general_theme = (UIThemeStyle *) *((uintptr_t *) pos);
    pos += sizeof(uintptr_t);

    const wchar_t* const theme_path = (wchar_t *) pos;
    str_move_to((const wchar_t **) &pos, L'\0');
    pos += sizeof(L'\0');

    const Camera* camera = (Camera *) *((uintptr_t *) pos);

    return cmd_ui_load_sync(
        cb,
        &scene->ui_layout, layout_path,
        general_theme,
        &scene->ui_theme, theme_path,
        camera
    );
}

inline
void thrd_cmd_ui_load(
    AppCmdBuffer* const __restrict cb,
    SceneInfo* __restrict scene_info,
    const wchar_t* __restrict layout_path,
    UIThemeStyle* __restrict general_theme,
    const wchar_t* __restrict theme_path,
    const Camera* const __restrict camera,
    CommandFunction callback
) NO_EXCEPT
{
    Command cmd;
    cmd.type = CMD_UI_LOAD;
    cmd.callback = callback;
    byte* pos = cmd.data;

    // Scene info pointer
    *((uintptr_t *) pos) = (uintptr_t) scene_info;
    pos += sizeof(uintptr_t);

    // Layout path
    pos += str_copy_until((wchar_t *) pos, layout_path, L'\0') * sizeof(wchar_t);
    *pos = L'\0';
    pos += sizeof(L'\0');

    // General theme pointer
    *((uintptr_t *) pos) = (uintptr_t) general_theme;
    pos += sizeof(uintptr_t);

    // Theme path
    pos += str_copy_until((wchar_t *) pos, theme_path, L'\0') * sizeof(wchar_t);
    *pos = L'\0';
    pos += sizeof(L'\0');

    // Camera pointer
    *((uintptr_t *) pos) = (uintptr_t) camera;

    thrd_cmd_insert(cb, &cmd);
}

#endif