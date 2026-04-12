/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_BUFFER_UI_PRODUCER_H
#define COMS_COMMAND_BUFFER_UI_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemoryT.h"
#include "../ui/UILayout.cpp"
#include "../ui/UITheme.cpp"
#include "AppCommand.h"
#include "CmdGeneralProducer.h"

inline
void thrd_cmd_ui_load(
    ChunkMemoryT<AppCommand>* const cb,
    const wchar_t* const __restrict layout_path,
    const wchar_t* const __restrict theme_path,
    UIThemeStyle* const __restrict general_theme,
    SceneInfo* const scene_info,
    GpuApiType gpu_api_type,
    RingMemory* const ring,
    AppCommandFunction callback
) NO_EXCEPT
{
    AppCommand cmd;
    cmd.type = CMD_UI_LOAD;
    cmd.callback = callback;

    wcscpy(cmd.layout_body.layout_path, layout_path);
    wcscpy(cmd.layout_body.theme_path, theme_path);

    cmd.layout_body.general_theme = general_theme;

    cmd.layout_body.scene_info = scene_info;
    cmd.layout_body.gpu_api_type = gpu_api_type;
    cmd.layout_body.ring = ring;

    thrd_cmd_insert(cb, &cmd);
}

#endif