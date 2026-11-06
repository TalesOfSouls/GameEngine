#pragma once
#ifndef COMS_UI_LAYOUT_TXT_H
#define COMS_UI_LAYOUT_TXT_H

#include "../stdlib/Stdlib.h"
#include "../object/TextureAtlas.h"
#include "UITheme.h"
#include "../localization/Language.h"

// Version of the .layouttxt format (@see UILayoutTxt.cpp)
#define UI_LAYOUT_TXT_VERSION 2

/**
 * TEXT FORMAT: .layouttxt, version UI_LAYOUT_TXT_VERSION
 *
 * Defines the element tree of a scene/hud. The syntax is the ui document syntax
 * (@see UIDocument.h):
 *
 *      version 2
 *
 *      // Comment (only full line comments)
 *      window test_window              <- "type [name]", the name is used by ui_get_element()
 *          style: window_default       <- styles of the theme, applied before the own properties
 *          anchor: top left
 *          pos: 512 256
 *          size: 560 480
 *          title: Test window
 *          $close                      <- part: fixed child created by the element itself
 *              on_click: window_close
 *
 *          label status                <- child element (more indentation than the parent)
 *              text: Status: -
 *
 *      indentation:  spaces, a tab counts as 4 (@see UI_DOCUMENT_TAB_WIDTH)
 *      block:        "type [name]"
 *      property:     "key: value"
 *      part:         "$part [argument]"
 *
 * Value syntax
 *      numbers       12, -3.5, 50% (= relative to the parent for pos/size)
 *      size          0 = content size, negative = parent size minus the value
 *      colors        #RRGGBBAA or #RRGGBB
 *      lists         "Option A | Option B | Option C"
 *      text          \n new line, \t tab, \\ backslash, \@ literal @
 *      strings       @key  = string of the language of this file (UILayoutTxtContext::language)
 *                    @@key = global string of the application (UILayoutTxtContext::language_global)
 *      flags         hitbox interactable movable resizable hoverable selectable ignore_hit always_on_top
 *                    +flag / -flag add/remove, otherwise the flags are replaced
 *      anchor        "<parent point> [/ <element point>]", points: left|center|right top|center|bottom,
 *                    e.g. "top right" or "top right / top left"
 *
 * Order in which the properties are applied
 *      1. create arguments (e.g. columns of a table, @see ui_txt_is_create_key)
 *      2. type default styles of the themes ("@table")
 *      3. styles listed in "style:" (in this order)
 *      4. own properties and parts (file order)
 *      5. child elements
 *
 * Element types: window, panel, label, button, image, cursor, custom, checkbox,
 * radio, slider, input, textarea, select, tab, table, colorpicker, tooltip.
 * The authoritative list of the properties per element type is the table at the
 * top of UILayoutTxt.cpp (all, label, button, ..., tooltip) - keep both in sync.
 *
 * BINARY FORMAT: .layoutbin, version UI_LAYOUT_VERSION
 * @see UILayout.h for the byte layout (layout_to_data / layout_from_data).
 */

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
