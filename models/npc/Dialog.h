#include "../../stdlib/Types.h"
#include "../mob/player/Reputation.h"
#include "DialogType.h"
#include "CharacterMood.h"

struct Dialog {
    // The id of this dialog
    uint64 id;

    // Sometimes you want to output a specific message,
    // sometimes a random message from a group of messages
    // This is different from the id since the same message could be used in different dialogs
    uint64 message_id;
    uint32 message_group_id;

    uint32 npc_id;

    // Sometimes dialog is for an entire class of NPCs
    // Maybe the npc doesn't even exist at all times -> no id available
    uint32 npc_type;

    // WeatherStates
    uint32 weather_flags;

    // Effects the amount of snow/rain/clouds etc.
    uint8 weather_intensity;

    // How long until this message can be shown again
    uint8 cooldown;

    uint8 volume;

    uint8 speed;

    // Pause until next sentence
    uint8 pause;

    uint8 player_level;

    // How many players are in a group?
    uint8 group_size;

    // Are there multiple players around (doesn't have to be a group)
    uint8 crowd_size;

    Reputation player;
    Reputation region;
    Reputation continent;
    Reputation global;

    CharacterMood mood;

    DialogType dialog_type;
};