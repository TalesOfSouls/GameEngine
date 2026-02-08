/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_H
#define COMS_COMMAND_H

#include "../stdlib/Stdlib.h"
#include "../ui/UILayout.h"
#include "../ui/UITheme.h"
#include "../scene/SceneInfo.h"

// WARNING: INTERNAL types are only intended for internal use (not by other programmers/systems)
//          Currently the INTERNAL command type is never used at least not in terms of the normal command pool iteration
enum AppCommandType : uint8 {
    CMD_FUNC_RUN,
    CMD_ASSET_ENQUEUE,
    CMD_ASSET_LOAD,
    CMD_FILE_LOAD,
    CMD_FONT_LOAD,
    CMD_INTERNAL_FONT_CREATE,
    CMD_TEXTURE_LOAD,
    CMD_INTERNAL_TEXTURE_CREATE,
    CMD_AUDIO_PLAY,
    CMD_INTERNAL_AUDIO_ENQUEUE,
    CMD_SHADER_LOAD,
    CMD_UI_LOAD,
};

typedef void* (*AppCommandFunction)(void* data);

struct CmdFunctionBody {
    AppCommandFunction func;
    void* parameter;
};

struct CmdFileBody {
    AppCommandFunction callback;

    union {
        const wchar_t* file_path;
        FileToLoad* file_to_load;
    };
};

struct CmdAssetBody {
    union {
        int32 asset_id;
        const wchar_t* asset_path;
    };
};

struct CmdAudioBody {
    int32 mixer_id;

    CmdAssetBody asset;
};

struct CmdFontBody {
    CmdAssetBody asset;
};

struct CmdTextureBody {
    CmdAssetBody asset;
};

struct CmdLayoutBody {
    UIThemeStyle* general_theme;

    UILayout* layout;
    wchar_t layout_path[64];

    UIThemeStyle* theme;
    wchar_t theme_path[64];

    SceneInfo* scene_info;
};

// Another name for this concept is event queue and the command below is a generic event
struct AppCommand {
    AppCommandFunction callback;
    AppCommandType type;

    union {
        CmdAssetBody asset_body;
        CmdAudioBody audio_body;
        CmdFontBody font_body;
        CmdFunctionBody func_body;
        CmdFileBody file_body;
        CmdTextureBody texture_body;
        CmdLayoutBody layout_body;
    };
};

#endif