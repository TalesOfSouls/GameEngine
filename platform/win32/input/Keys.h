/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_KEYS_H
#define COMS_PLATFORM_WIN32_INPUT_KEYS_H

#include "../../../stdlib/Stdlib.h"
#include <windows.h>
#include <Winuser.h>

// @bug the order is bugged (there are some 0s too much/too little)
static CONSTEXPR const uint16 _vk_to_sc_table[256] = {
    /* 0x00 */ 0,
    /* 0x01 VK_LBUTTON   */ 0,      // mouse
    /* 0x02 VK_RBUTTON   */ 0,
    /* 0x03 VK_CANCEL    */ 0,
    /* 0x04 VK_MBUTTON   */ 0,
    /* 0x05 VK_XBUTTON1  */ 0,
    /* 0x06 VK_XBUTTON2  */ 0,
    /* 0x07 */ 0,
    /* 0x08 VK_BACK      */ 0x0E,
    /* 0x09 VK_TAB       */ 0x0F,
    /* 0x0A */ 0,
    /* 0x0B */ 0,
    /* 0x0C VK_CLEAR     */ 0,
    /* 0x0D VK_RETURN    */ 0x1C,
    /* 0x0E */ 0,
    /* 0x0F */ 0,

    /* 0x10 VK_SHIFT     */ 0,
    /* 0x11 VK_CONTROL   */ 0,
    /* 0x12 VK_MENU      */ 0,
    /* 0x13 VK_PAUSE     */ 0x45,
    /* 0x14 VK_CAPITAL   */ 0x3A,
    /* 0x15 */ 0,
    /* 0x16 */ 0,
    /* 0x17 */ 0,
    /* 0x18 */ 0,
    /* 0x19 */ 0,
    /* 0x1A */ 0,
    /* 0x1B VK_ESCAPE    */ 0x01,
    /* 0x1C */ 0,
    /* 0x1D */ 0,
    /* 0x1E */ 0,
    /* 0x1F */ 0,

    /* 0x20 VK_SPACE     */ 0x39,
    /* 0x21 VK_PRIOR     */ 0x149,
    /* 0x22 VK_NEXT      */ 0x151,
    /* 0x23 VK_END       */ 0x14F,
    /* 0x24 VK_HOME      */ 0x147,
    /* 0x25 VK_LEFT      */ 0x14B,
    /* 0x26 VK_UP        */ 0x148,
    /* 0x27 VK_RIGHT     */ 0x14D,
    /* 0x28 VK_DOWN      */ 0x150,
    /* 0x29 */ 0,
    /* 0x2A */ 0,
    /* 0x2B */ 0,
    /* 0x2C VK_SNAPSHOT */ 0x137,
    /* 0x2D VK_INSERT    */ 0x152,
    /* 0x2E VK_DELETE    */ 0x153,
    /* 0x2F VK_HELP */ 0,

    /* 0x30 VK_0 */ 0x0B,
    /* 0x31 VK_1 */ 0x02,
    /* 0x32 VK_2 */ 0x03,
    /* 0x33 VK_3 */ 0x04,
    /* 0x34 VK_4 */ 0x05,
    /* 0x35 VK_5 */ 0x06,
    /* 0x36 VK_6 */ 0x07,
    /* 0x37 VK_7 */ 0x08,
    /* 0x38 VK_8 */ 0x09,
    /* 0x39 VK_9 */ 0x0A,

    /* 0x41 VK_A */ 0x1E,
    /* 0x42 VK_B */ 0x30,
    /* 0x43 VK_C */ 0x2E,
    /* 0x44 VK_D */ 0x20,
    /* 0x45 VK_E */ 0x12,
    /* 0x46 VK_F */ 0x21,
    /* 0x47 VK_G */ 0x22,
    /* 0x48 VK_H */ 0x23,
    /* 0x49 VK_I */ 0x17,
    /* 0x4A VK_J */ 0x24,
    /* 0x4B VK_K */ 0x25,
    /* 0x4C VK_L */ 0x26,
    /* 0x4D VK_M */ 0x32,
    /* 0x4E VK_N */ 0x31,
    /* 0x4F VK_O */ 0x18,
    /* 0x50 VK_P */ 0x19,
    /* 0x51 VK_Q */ 0x10,
    /* 0x52 VK_R */ 0x13,
    /* 0x53 VK_S */ 0x1F,
    /* 0x54 VK_T */ 0x14,
    /* 0x55 VK_U */ 0x16,
    /* 0x56 VK_V */ 0x2F,
    /* 0x57 VK_W */ 0x11,
    /* 0x58 VK_X */ 0x2D,
    /* 0x59 VK_Y */ 0x15,
    /* 0x5A VK_Z */ 0x2C,

    /* 0x5B VK_LWIN */ 0x15B,
    /* 0x5C VK_RWIN */ 0x15C,
    /* 0x5D VK_APPS */ 0x15D,

    /* 0x60 VK_NUMPAD0 */ 0x52,
    /* 0x61 VK_NUMPAD1 */ 0x4F,
    /* 0x62 VK_NUMPAD2 */ 0x50,
    /* 0x63 VK_NUMPAD3 */ 0x51,
    /* 0x64 VK_NUMPAD4 */ 0x4B,
    /* 0x65 VK_NUMPAD5 */ 0x4C,
    /* 0x66 VK_NUMPAD6 */ 0x4D,
    /* 0x67 VK_NUMPAD7 */ 0x47,
    /* 0x68 VK_NUMPAD8 */ 0x48,
    /* 0x69 VK_NUMPAD9 */ 0x49,

    /* 0x6A VK_MULTIPLY */ 0x37,
    /* 0x6B VK_ADD      */ 0x4E,
    /* 0x6C */ 0,
    /* 0x6D VK_SUBTRACT */ 0x4A,
    /* 0x6E VK_DECIMAL  */ 0x53,
    /* 0x6F VK_DIVIDE   */ 0x135,

    /* 0x70 VK_F1  */  0x3B,
    /* 0x71 VK_F2  */  0x3C,
    /* 0x72 VK_F3  */  0x3D,
    /* 0x73 VK_F4  */  0x3E,
    /* 0x74 VK_F5  */  0x3F,
    /* 0x75 VK_F6  */  0x40,
    /* 0x76 VK_F7  */  0x41,
    /* 0x77 VK_F8  */  0x42,
    /* 0x78 VK_F9  */  0x43,
    /* 0x79 VK_F10 */  0x44,
    /* 0x7A VK_F11 */  0x57,
    /* 0x7B VK_F12 */  0x58,
    /* 0x7B VK_F13 */  0x58,
    /* 0x7B VK_F14 */  0x58,
    /* 0x7B VK_F15 */  0x58,
    /* 0x7B VK_F16 */  0x58,
    /* 0x7B VK_F17 */  0x58,
    /* 0x7B VK_F18 */  0x58,
    /* 0x7B VK_F19 */  0x58,
    /* 0x7B VK_F20 */  0x58,
    /* 0x7B VK_F21 */  0x58,
    /* 0x7B VK_F22 */  0x58,
    /* 0x7B VK_F23 */  0x58,
    /* 0x7B VK_F24 */  0x58,

    0, 0, 0, 0, 0, 0, 0,
    0,

    /* 0x7B VK_NUMLOCK */ 0,
    /* 0x7B VK_SCROLL */ 0x46,

    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* modifiers */
    /* 0xA0 VK_LSHIFT */ 0x2A,
    /* 0xA1 VK_RSHIFT */ 0x36,
    /* 0xA2 VK_LCONTROL */ 0x1D,
    /* 0xA3 VK_RCONTROL */ 0x11D,
    /* 0xA4 VK_LMENU */ 0x38,
    /* 0xA5 VK_RMENU */ 0x138,

    /* 0xA7 VK_BROWSER_FORWARD */ 0,
    /* 0xA8 VK_BROWSER_REFRESH */ 0,
    /* 0xA9 VK_BROWSER_STOP */ 0,
    /* 0xAA VK_BROWSER_SEARCH */ 0,
    /* 0xAB VK_BROWSER_FAVORITES */ 0,
    /* 0xAC VK_BROWSER_HOME */ 0,
    /* 0xAD VK_VOLUME_MUTE */ 0,
    /* 0xAE VK_VOLUME_DOWN */ 0,
    /* 0xAF VK_VOLUME_UP */ 0,
    /* 0xB0 VK_MEDIA_NEXT_TRACK */ 0,
    /* 0xB1 VK_MEDIA_PREV_TRACK */ 0,
    /* 0xB2 VK_MEDIA_STOP */ 0,
    /* 0xB3 VK_MEDIA_PLAY_PAUSE */ 0,
    /* 0xB4 VK_LAUNCH_MAIL */ 0,
    /* 0xB5 VK_LAUNCH_MEDIA_SELECT */ 0,
    /* 0xB6 VK_LAUNCH_APP1 */ 0,
    /* 0xB7 VK_LAUNCH_APP2 */ 0,
    /* 0xB8 */ 0,
    /* 0xB9 */ 0,
    /* 0xBA VK_OEM_1 */ 0,
    /* 0xBB VK_OEM_PLUS */ 0,
    /* 0xBC VK_OEM_COMMA */ 0,
    /* 0xBD VK_OEM_MINUS */ 0,
    /* 0xBE VK_OEM_PERIOD */ 0,
    /* 0xBF VK_OEM_2 */ 0,
    /* 0xC0 VK_OEM_3 */ 0,
    /* 0xC1 */ 0,
    /* 0xC2 */ 0,
    /* 0xC3 VK_GAMEPAD_A */ 0,
    /* 0xC4 VK_GAMEPAD_B */ 0,
    /* 0xC5 VK_GAMEPAD_X */ 0,
    /* 0xC6 VK_GAMEPAD_Y */ 0,
    /* 0xC7 VK_GAMEPAD_RIGHT_SHOULDER */ 0,
    /* 0xC8 VK_GAMEPAD_LEFT_SHOULDER */ 0,
    /* 0xC9 VK_GAMEPAD_LEFT_TRIGGER */ 0,
    /* 0xCA VK_GAMEPAD_RIGHT_TRIGGER */ 0,
    /* 0xCB VK_GAMEPAD_DPAD_UP */ 0,
    /* 0xCC VK_GAMEPAD_DPAD_DOWN */ 0,
    /* 0xCD VK_GAMEPAD_DPAD_LEFT */ 0,
    /* 0xCE VK_GAMEPAD_DPAD_RIGHT */ 0,
    /* 0xCF VK_GAMEPAD_MENU */ 0,
    /* 0xD0 VK_GAMEPAD_VIEW */ 0,
    /* 0xD1 VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON */ 0,
    /* 0xD2 VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON */ 0,
    /* 0xD3 VK_GAMEPAD_LEFT_THUMBSTICK_UP */ 0,
    /* 0xD4 VK_GAMEPAD_LEFT_THUMBSTICK_DOWN */ 0,
    /* 0xD5 VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT */ 0,
    /* 0xD6 VK_GAMEPAD_LEFT_THUMBSTICK_LEFT */ 0,
    /* 0xD7 VK_GAMEPAD_RIGHT_THUMBSTICK_UP */ 0,
    /* 0xD8 VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN */ 0,
    /* 0xD9 VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT */ 0,
    /* 0xDA VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT */ 0,
    /* 0xDB VK_OEM_4 */ 0,
    /* 0xDC VK_OEM_5 */ 0,
    /* 0xDD VK_OEM_6 */ 0,
    /* 0xDE VK_OEM_7 */ 0,
    /* 0xDF VK_OEM_8 */ 0,
    /* 0xE0 */ 0,
    /* 0xE1 */ 0,
    /* 0xE2 VK_OEM_102 */ 0,
    /* 0xE3 */ 0,
    /* 0xE4 */ 0,
    /* 0xE5 VK_PROCESSKEY */ 0,
    /* 0xE6 */ 0,
    /* 0xE7 VK_PACKET */ 0,
    /* 0xE8 */ 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
    /* 0xF6 VK_ATTN */ 0,
    /* 0xF7 VK_CRSEL */ 0,
    /* 0xF8 VK_EXSEL */ 0,
    /* 0xF9 VK_EREOF */ 0,
    /* 0xFA VK_PLAY */ 0,
    /* 0xFB VK_ZOOM */ 0,
    /* 0xFC VK_NONAME */ 0,
    /* 0xFD VK_PA1 */ 0,
    /* 0xFE VK_OEM_CLEAR */ 0,
};

int32 raw_input_to_scan_code(USHORT make_code, USHORT flags)
{
    int32 sc = make_code;

    if (flags & RI_KEY_E0) {
        sc |= 0x100;
    }

    // Pause / special
    if (flags & RI_KEY_E1) {
        sc |= 0x200;
    }

    return sc;
}

int32 scan_code_to_vk(int32 sc)
{
    HKL layout = GetKeyboardLayout(0);

    int32 vk;
    if (sc & 0x100) {
        vk = MapVirtualKeyEx(sc & 0xFF, MAPVK_VSC_TO_VK_EX, layout);
    } else {
        vk = MapVirtualKeyEx(sc, MAPVK_VSC_TO_VK, layout);
    }

    return vk;
}


int32 vk_to_scan_code(int32 vk)
{
    HKL layout = GetKeyboardLayout(0);

    int32 sc = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC_EX, layout);
    if (!sc) {
        return 0;
    }

    // If it's an extended key, set the extended bit
    switch (vk) {
        case 0x2D: FALLTHROUGH; // insert
        case 0x2E: FALLTHROUGH; // delete
        case 0x24: FALLTHROUGH; // home
        case 0x23: FALLTHROUGH; // end
        case 0x21: FALLTHROUGH; // page up
        case 0x22: FALLTHROUGH; // page down
        case 0x25: FALLTHROUGH; // left
        case 0x27: FALLTHROUGH; // right
        case 0x26: FALLTHROUGH; // up
        case 0x28: FALLTHROUGH; // down
        case 0x6F: FALLTHROUGH; // divide
        case 0x90: FALLTHROUGH; // numlock
        case 0xA3: FALLTHROUGH; // right ctrl
        case 0xA5: FALLTHROUGH; // right menu
        case 0x5D: FALLTHROUGH; // application
        case 0x5B: FALLTHROUGH; // left windows
        case 0x5C: // right windows
            sc |= 0x100; // Extended key flag
            break;
    }

    return sc;
}

#endif