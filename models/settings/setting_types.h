#ifndef TOS_MODELS_SETTING_TYPES_H
#define TOS_MODELS_SETTING_TYPES_H

#define SETTING_TYPE_GPU_CUSTOM 0x0
#define SETTING_TYPE_GPU_VLOW 0x1
#define SETTING_TYPE_GPU_LOW 0x2
#define SETTING_TYPE_GPU_MEDIUM 0x3
#define SETTING_TYPE_GPU_HIGH 0x4
#define SETTING_TYPE_GPU_VHIGH 0x5
#define SETTING_TYPE_GPU_ULTRA 0x6
#define SETTING_TYPE_GPU_NEXTGEN 0x7

#define SETTING_TYPE_GPU_API_NONE 0x0
#define SETTING_TYPE_GPU_API_DIRECTX11 0x1
#define SETTING_TYPE_GPU_API_DIRECTX12 0x2
#define SETTING_TYPE_GPU_API_OPENGL 0x3

#define SETTING_TYPE_PERSPECTIVE_FIRST 0x00
#define SETTING_TYPE_PERSPECTIVE_THIRD 0x01
#define SETTING_TYPE_PERSPECTIVE_ISOMETRIC 0x02

#define SETTING_TYPE_ANTI_ALIASING_TAA 0x01
#define SETTING_TYPE_ANTI_ALIASING_SSAA 0x02
#define SETTING_TYPE_ANTI_ALIASING_MSAA 0x03
#define SETTING_TYPE_ANTI_ALIASING_FXAA 0x04

#define SETTING_TYPE_SYNC_V 0x1
#define SETTING_TYPE_SYNC_ADAPTIVE 0x2
#define SETTING_TYPE_SYNC_FAST 0x3

#define SETTING_TYPE_ASPEC_RATIO_4x3 0x00
#define SETTING_TYPE_ASPEC_RATIO_16x9 0x01
#define SETTING_TYPE_ASPEC_RATIO_16x10 0x02
#define SETTING_TYPE_ASPEC_RATIO_21x9 0x03

#define SETTING_TYPE_SCREEN_RESOLUTION_800x600 0x00
#define SETTING_TYPE_SCREEN_RESOLUTION_1024x768 0x01
#define SETTING_TYPE_SCREEN_RESOLUTION_1280x720 0x02
#define SETTING_TYPE_SCREEN_RESOLUTION_1280x800 0x03
#define SETTING_TYPE_SCREEN_RESOLUTION_1280x1024 0x04
#define SETTING_TYPE_SCREEN_RESOLUTION_1360x768 0x05
#define SETTING_TYPE_SCREEN_RESOLUTION_1366x768 0x06
#define SETTING_TYPE_SCREEN_RESOLUTION_1440x900 0x07
#define SETTING_TYPE_SCREEN_RESOLUTION_1536x864 0x08
#define SETTING_TYPE_SCREEN_RESOLUTION_1600x900 0x09
#define SETTING_TYPE_SCREEN_RESOLUTION_1600x1200 0x0A
#define SETTING_TYPE_SCREEN_RESOLUTION_1680x1050 0x0B
#define SETTING_TYPE_SCREEN_RESOLUTION_1920x1080 0x0C
#define SETTING_TYPE_SCREEN_RESOLUTION_1920x1200 0x0D
#define SETTING_TYPE_SCREEN_RESOLUTION_2048x1152 0x0E
#define SETTING_TYPE_SCREEN_RESOLUTION_2048x1536 0x0F
#define SETTING_TYPE_SCREEN_RESOLUTION_2560x1080 0x10
#define SETTING_TYPE_SCREEN_RESOLUTION_2560x1440 0x11
#define SETTING_TYPE_SCREEN_RESOLUTION_2560x1600 0x12
#define SETTING_TYPE_SCREEN_RESOLUTION_3440x1440 0x13
#define SETTING_TYPE_SCREEN_RESOLUTION_3840x2160 0x14

#define SETTING_TYPE_WINDOW_CHAT 0x01
#define SETTING_TYPE_WINDOW_MAP 0x02
#define SETTING_TYPE_WINDOW_RANKING 0x03
#define SETTING_TYPE_WINDOW_SHOP 0x04
#define SETTING_TYPE_WINDOW_STATS 0x05
#define SETTING_TYPE_WINDOW_GROUPS 0x06
#define SETTING_TYPE_WINDOW_PET 0x07 // Pet "tamagochi" window

#define SETTING_TYPE_WINDOW_MODE_FULLSCREEN 0x00
#define SETTING_TYPE_WINDOW_MODE_WINDOWED_FULLSCREEN 0x01
#define SETTING_TYPE_WINDOW_MODE_WINDOWED 0x02

#define SETTING_TYPE_SIMD_128 1
#define SETTING_TYPE_SIMD_256 2
#define SETTING_TYPE_SIMD_512 3

#define SETTING_TYPE_DISABLED 0x00
#define SETTING_TYPE_UNLIMITED 0x00

#define SETTING_UI_VISIBILITY_FPS 1
#define SETTING_UI_VISIBILITY_DEBUG 2
#define SETTING_UI_VISIBILITY_WIREFRAME 4

#define SETTING_INPUT_DEVICE_TYPE_MOUSE_KEYBOARD 1
#define SETTING_INPUT_DEVICE_TYPE_CONTROLLER 2

#endif