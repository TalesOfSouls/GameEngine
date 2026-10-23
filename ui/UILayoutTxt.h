#pragma once
#ifndef COMS_UI_LAYOUT_TXT_H
#define COMS_UI_LAYOUT_TXT_H

#include "../stdlib/Stdlib.h"
#include "../object/TextureAtlas.h"
#include "UITheme.h"
#include "../localization/Language.h"

// Version of the .layouttxt format (@see UILayoutTxt.cpp)
#define UI_LAYOUT_TXT_VERSION 2

// Name -> id, e.g. function names of the layout file -> UICore::update_func (@see UILayoutTxtContext)
struct UINameId {
    const char* name;
    int32 id;
};

/**
 * Everything the layout file references by name (@see ui_layout_from_txt)
 * Unknown names are reported (LOG) and ignored
 */
struct UILayoutTxtContext {
    // Styles, later themes overwrite earlier themes (e.g. general theme, scene theme)
    const UITheme* themes[4];
    int32 theme_count;

    // Images (image, border_image), the id is the atlas element
    const TextureAtlas* atlas;
    const UINameId* images;
    int32 image_count;

    // Functions, the id is the 1-based index into UILayout::update/render/on_actions
    const UINameId* update_functions;
    int32 update_function_count;

    const UINameId* render_functions;
    int32 render_function_count;

    const UINameId* actions;
    int32 action_count;

    // Strings of the layout file ("@key", @see ui_txt_value_resolve), NULL = no layout language
    // Strings in a theme file use the language of the theme (UITheme::language)
    const Language* language;

    // Global strings of the application ("@@key"), NULL = none
    const Language* language_global;
};

#endif
