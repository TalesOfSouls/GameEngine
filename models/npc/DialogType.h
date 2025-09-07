/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_NPC_DIALOG_TYPE_H
#define COMS_MODELS_NPC_DIALOG_TYPE_H

enum DialogType {
    DIALOG_TYPE_QUEST,
    DIALOG_TYPE_BACKGROUND, // Background noise
    DIALOG_TYPE_JOB, // Job specific e.g. trainer instructions, merchant message
    DIALOG_TYPE_RESPONSE, // Response to player action
};

#endif