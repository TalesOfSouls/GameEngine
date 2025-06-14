#ifndef COMS_UI_ANIMATION_H
#define COMS_UI_ANIMATION_H

#include "../stdlib/Types.h"
#include "../animation/AnimationEaseType.h"

struct UIAnimationState {
    // When did the animation start?
    uint32 start;

    // Element specific use (e.g. cursor uses 0/1 for visible/invisible for blinking)
    // The highest bit indicates if the animation is active or not
    int16 state;

    // What is our last keyframe for reference?
    byte keyframe;

    // Currently active animation
    byte active_animation;
};

struct UIAnimation {
    // For which transition is this animation used?
    UIStyleType style_old;
    UIStyleType style_new;

    // This allows for a maximum duration of 65536ms = 65.536s
    uint16 duration;
    AnimationEaseType anim_type;

    // The last keyframe is always the style associated with style_new
    // This means that with 0 keyframes and a duration of 'Xs' the animation is interpolated between style_old and style_new
    // In other words the keyframes below only define ADDITIONAL steps in between style_old and style_new
    // The keyframes are defined in the theme file
    byte keyframe_count;

    // The dynamic array containing the offsets into the keyframes come directly after UIAnimation
    // UIElementDetail* keyframes;
};

#endif