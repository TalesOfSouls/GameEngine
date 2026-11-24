/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMMAND_BUFFER_UI_PRODUCER_H
#define COMS_COMMAND_BUFFER_UI_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ThrdChunkMemoryT.h"
#include "../memory/ChunkMemory.cpp"
#include "../ui/UILayout.cpp"
#include "../ui/UITheme.cpp"
#include "AppCommand.h"
#include "CmdGeneralProducer.h"

// @question Why are we passing all this data instead of just cb? this is mental
inline
void thrd_cmd_ui_load(
    AppCmdBuffer* const __restrict cb,
    void* app,
    AssetManagementSystem* const __restrict ams,
    const char layout_path[64],
    const char theme_path[64],
    UITheme* const __restrict general_theme,
    UILayout* const __restrict layout,
    UITheme* const __restrict theme,
    GpuApiType gpu_api_type,
    AppCommandFunction callback,
    const char* const language_code = NULL
) NO_EXCEPT
{
    AppCommand cmd = {};
    cmd.type = CMD_UI_LOAD;
    cmd.callback = callback;

    cmd.layout_body.app = app;
    cmd.layout_body.ams = ams;

    str_copy(cmd.layout_body.layout_path, layout_path);
    str_copy(cmd.layout_body.theme_path, theme_path);

    if (language_code) {
        str_copy(cmd.layout_body.language, language_code, oms_min((int32) str_length(language_code), (int32) ARRAY_COUNT(cmd.layout_body.language) - 1));
    }

    cmd.layout_body.general_theme = general_theme;

    cmd.layout_body.layout = layout;
    cmd.layout_body.theme = theme;

    cmd.layout_body.gpu_api_type = gpu_api_type;
    cmd.layout_body.mem = cb->mem;

    thrd_cmd_insert(&cb->commands, &cmd);
}

#endif