/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_GUI_UTILS_H
#define COMS_PLATFORM_WIN32_GUI_UTILS_H

#include <windows.h>
#include "Window.h"
#include "../../stdlib/Types.h"
#include "../../utils/TestUtils.h"

// @question Shouldn't this function and the next one accept a parameter of what to add/remove?
inline
void window_remove_style(Window* w)
{
    LONG_PTR style = GetWindowLongPtrA(w->hwnd, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    SetWindowLongPtr(w->hwnd, GWL_STYLE, style);
}

inline
void window_add_style(Window* w)
{
    LONG_PTR style = GetWindowLongPtrA(w->hwnd, GWL_STYLE);
    style |= WS_OVERLAPPEDWINDOW;
    SetWindowLongPtr(w->hwnd, GWL_STYLE, style);
}

inline
void monitor_resolution(const Window* __restrict w, v2_int32* __restrict resolution)
{
    resolution->width = GetDeviceCaps(w->hdc, HORZRES);
    resolution->height = GetDeviceCaps(w->hdc, VERTRES);
}

inline
void monitor_resolution(Window* w)
{
    w->width = (uint16) GetDeviceCaps(w->hdc, HORZRES);
    w->height = (uint16) GetDeviceCaps(w->hdc, VERTRES);
}

inline
void window_resolution(Window* w)
{
    RECT rect;
    GetClientRect(w->hwnd, &rect);

    w->width = (uint16) (rect.right - rect.left);
    w->height = (uint16) (rect.bottom - rect.top);
}

inline
void window_fullscreen(Window* w)
{
    monitor_resolution(w);
    w->x = 0;
    w->y = 0;

    window_remove_style(w);
    SetWindowPos(w->hwnd, HWND_TOP, 0, 0, w->width, w->height, SWP_NOACTIVATE | SWP_NOZORDER);
}

inline
void window_restore(Window* w)
{
    window_restore_state(w);

    SetWindowLongPtr(w->hwnd, GWL_STYLE, w->state_old.style);
    SetWindowPos(
        w->hwnd, HWND_TOP,
        w->state_old.x, w->state_old.y,
        w->state_old.width, w->state_old.height,
        SWP_NOACTIVATE | SWP_NOZORDER
    );
}

void window_create(Window* __restrict window, void* proc)
{
    ASSERT_SIMPLE(proc);

    WNDPROC wndproc = (WNDPROC) proc;
    WNDCLASSEXA wc = {};

    if (!window->hInstance) {
        window->hInstance = GetModuleHandle(0);
    }

    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = window->hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = (LPCSTR) window->name;

    if (!RegisterClassExA(&wc)) {
        return;
    }

    if (window->is_fullscreen) {
        window->width  = (uint16) GetSystemMetrics(SM_CXSCREEN);
	    window->height = (uint16) GetSystemMetrics(SM_CYSCREEN);

        DEVMODE screen_settings;

        memset(&screen_settings, 0, sizeof(screen_settings));
		screen_settings.dmSize       = sizeof(screen_settings);
		screen_settings.dmPelsWidth  = (unsigned long) window->width;
		screen_settings.dmPelsHeight = (unsigned long) window->height;
		screen_settings.dmBitsPerPel = 32;
		screen_settings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);

        window->x = 0;
        window->y = 0;
    }

    window->hwnd = CreateWindowExA((DWORD) NULL,
        wc.lpszClassName, NULL,
        WS_OVERLAPPEDWINDOW,
        window->x, window->y,
        window->width,
        window->height,
        NULL, NULL, window->hInstance, window
    );

    ASSERT_SIMPLE(window->hwnd);
}

void window_open(Window* window)
{
    ShowWindow(window->hwnd, SW_SHOW);
    SetForegroundWindow(window->hwnd);
	SetFocus(window->hwnd);
    UpdateWindow(window->hwnd);

    window->state_changes |= WINDOW_STATE_CHANGE_FOCUS;
}

void window_close(Window* window)
{
    CloseWindow(window->hwnd);
    DestroyWindow(window->hwnd);
}

HBITMAP CreateBitmapFromRGBA(HDC hdc, const byte* rgba, int32 width, int32 height) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pbits;
    HBITMAP hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pbits, NULL, 0);
    if (hbitmap) {
        memcpy(pbits, rgba, width * height * 4);
    }

    return hbitmap;
}

#endif