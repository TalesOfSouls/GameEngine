#ifndef COMS_EVENT_H
#define COMS_EVENT_H

#include "../../stdlib/Types.h"
#include "../mob/monster/LootTable.h"
#include "EventTaskType.h"

enum QuestState {
    QUEST_STATE_NOT_STARTED,
    QUEST_STATE_IN_PROGRESS,
    QUEST_STATE_COMPLETED,
    QUEST_STATE_FAILED
};

// also used for conditions in event tasks
struct EventRequirement {
    int* events_finished; // These events need to be completed
    int events_finished_count;

    int* events_not_finished;
    int events_not_finished_count;

    int* events_started; // These events need to be started
    int events_started_count;

    int* events_not_started;
    int events_not_started_count;

    int char_level_above;
    int char_level_below;

    int group_size; // some quests require a certain group size

    int* proficiencies_above;
    int* proficiencies_above_level;
    int proficiencies_above_count;

    int* proficiencies_below;
    int* proficiencies_below_level;
    int proficiencies_below_count;

    int char_trait_type; // npc, region, global, ...
    int* char_trait_above;
    int* char_trait_above_level;
    int char_trait_above_count;

    int* char_trait_below;
    int* char_trait_below_level;
    int char_trait_below_count;

    int char_recognition_type; // npc, region, global, ...
    int* char_recognition_above;
    int* char_recognition_above_level;
    int char_recognition_above_count;

    int* char_recognition_below;
    int* char_recognition_below_level;
    int char_recognition_below_count;
};

struct EventReward {
    int score; // score required for this reward

    LootTable loot;
    bool reward_given;
};

struct EventTaskCollect {
    int64* items;
    int* required_amount;
    int* collected_amount;

    // @todo some data should be visual, some only internal
    // sometimes we want to show radi, sometimes not
    // some of the radi are also mob/obj spawn areas for internal purposes
    v3_f32* positions;
    f32* radi;
};

struct EventTaskKill {
    int64 mob;
    int required_kills;
    int kills_done;

    v3_f32* positions;
    f32* radi;
};

struct EventTaskAccompany {
    int64 npc;

    v3_f32* positions;
    f32* radi;

    bool reached_destination;
};

struct EventTaskDiscover {
    v3_f32* positions;
    f32* radi;

    bool* discovered;
};

struct EventTaskDeliver {
    // @todo first pickup items from location/npc?

    int64* items;
    int* required_amount;
    int* delivered_amount;

    // pickup location
    v3_f32* positions;
    f32* radi;

    // @todo destination

    bool* delivered;
};

struct EventTask {
    EventTaskType type;
    bool completed;
    union {
        EventTaskCollect collect;
        EventTaskKill kill;
        EventTaskAccompany accompany;
        EventTaskDiscover discover;
    };
};

struct Event {
    int64 id;
    QuestState state;
    int64 giver;

    char* title;
    char* description;

    EventTask* tasks;

    EventRequirement* requirements;
    EventReward* rewards;
};

#endif