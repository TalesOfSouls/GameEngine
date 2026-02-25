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
#include "../../stdlib/Stdlib.h"
#include "../../utils/Assert.h"
#include "../../utils/StringUtils.h"
#include "../../compiler/CompilerUtils.h"

// @question Shouldn't this function and the next one accept a parameter of what to add/remove?
FORCE_INLINE
void window_remove_style(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;

    LONG_PTR style = GetWindowLongPtrW(platform_window->hwnd, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    SetWindowLongPtr(platform_window->hwnd, GWL_STYLE, style);
}

FORCE_INLINE
void window_add_style(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;

    LONG_PTR style = GetWindowLongPtrW(platform_window->hwnd, GWL_STYLE);
    style |= WS_OVERLAPPEDWINDOW;
    SetWindowLongPtr(platform_window->hwnd, GWL_STYLE, style);
}

FORCE_INLINE
void physical_resolution(Window* const w) {
    w->dpi = (byte) GetDpiForWindow(((WindowPlatform *) w->platform_window)->hwnd);

    w->state_current.physical_width = (uint16) (((uint32) w->state_current.logical_width * (uint32) w->dpi) / 96);
    w->state_current.physical_height = (uint16) (((uint32) w->state_current.logical_height * (uint32) w->dpi) / 96);
}

FORCE_INLINE
void monitor_resolution(const Window* __restrict w, v2_int32* __restrict resolution) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;
    resolution->width = GetDeviceCaps(platform_window->hdc, HORZRES);
    resolution->height = GetDeviceCaps(platform_window->hdc, VERTRES);
}

FORCE_INLINE
void monitor_resolution(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;
    w->state_current.logical_width = (uint16) GetDeviceCaps(platform_window->hdc, HORZRES);
    w->state_current.logical_height = (uint16) GetDeviceCaps(platform_window->hdc, VERTRES);

    physical_resolution(w);
}

FORCE_INLINE
void window_resolution(Window* const w) NO_EXCEPT
{
    RECT rect;
    GetClientRect(((WindowPlatform *) w->platform_window)->hwnd, &rect);

    w->state_current.x = (uint16) rect.left;
    w->state_current.y = (uint16) rect.top;
    w->state_current.logical_width = (uint16) (rect.right - rect.left);
    w->state_current.logical_height = (uint16) (rect.bottom - rect.top);

    physical_resolution(w);

    if (!w->state_current.physical_width || !w->state_current.physical_height) {
        w->state_flag |= WINDOW_STATE_FLAG_DIMENSIONLESS;
    } else {
        w->state_flag &= ~WINDOW_STATE_FLAG_DIMENSIONLESS;
    }
}

inline
void window_fullscreen(Window* const w) NO_EXCEPT
{
    monitor_resolution(w);
    w->state_current.x = 0;
    w->state_current.y = 0;

    SetWindowPos(
        ((WindowPlatform *) w->platform_window)->hwnd, HWND_TOP, 0, 0,
        w->state_current.logical_width, w->state_current.logical_height,
        SWP_NOACTIVATE | SWP_NOZORDER
    );
}

inline
void window_restore(Window* const w) NO_EXCEPT
{
    window_restore_state(w);

    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;

    SetWindowLongPtr(platform_window->hwnd, GWL_STYLE, w->state_old.style);
    SetWindowPos(
        platform_window->hwnd, HWND_TOP,
        w->state_old.x, w->state_old.y,
        w->state_old.logical_width, w->state_old.logical_height,
        SWP_NOACTIVATE | SWP_NOZORDER
    );
}

void window_create(Window* const __restrict window, void* const __restrict proc) NO_EXCEPT
{
    ASSERT_TRUE(proc);

    WindowPlatform* const platform_window = (WindowPlatform *) window->platform_window;

    WNDPROC wndproc = (WNDPROC) proc;

    if (!platform_window->hInstance) {
        platform_window->hInstance = GetModuleHandle(0);
    }

    wchar_t title[64];
    char_to_wchar(title, window->name, ARRAY_COUNT(title) - 1);

    WNDCLASSEXW wc = {
        sizeof(WNDCLASSEXW), // .cbSize =
        CS_OWNDC, // .style =
        wndproc, // .lpfnWndProc =
        0, // .cbClsExtra =
        0, // .cbWndExtra =
        platform_window->hInstance, // .hInstance =
        NULL, // .hIcon =
        LoadCursor(NULL, IDC_ARROW), // .hCursor =
        NULL, // .hbrBackground =
        NULL, // .lpszMenuName =
        (LPCWSTR) title, // .lpszClassName =
    };

    if (!RegisterClassExW(&wc)) {
        return;
    }

    if (window->state_flag & WINDOW_STATE_FLAG_FULLSCREEN) {
        window->state_current.logical_width  = (uint16) GetSystemMetrics(SM_CXSCREEN);
	    window->state_current.logical_height = (uint16) GetSystemMetrics(SM_CYSCREEN);

        DEVMODE screen_settings;

        memset(&screen_settings, 0, sizeof(screen_settings));
		screen_settings.dmSize       = sizeof(screen_settings);
		screen_settings.dmPelsWidth  = (unsigned long) window->state_current.logical_width;
		screen_settings.dmPelsHeight = (unsigned long) window->state_current.logical_height;
		screen_settings.dmBitsPerPel = 32;
		screen_settings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);

        window->state_current.x = 0;
        window->state_current.y = 0;
    }

    platform_window->hwnd = CreateWindowExW((DWORD) NULL,
        wc.lpszClassName, NULL,
        WS_OVERLAPPEDWINDOW,
        window->state_current.x, window->state_current.y,
        window->state_current.logical_width,
        window->state_current.logical_height,
        NULL, NULL, platform_window->hInstance, window
    );

    window_resolution(window);

    ASSERT_TRUE(platform_window->hwnd);
}

inline
void window_open(Window* const window) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) window->platform_window;
    ShowWindow(platform_window->hwnd, SW_SHOW);
    SetForegroundWindow(platform_window->hwnd);
	SetFocus(platform_window->hwnd);
    UpdateWindow(platform_window->hwnd);

    window->state_changes |= WINDOW_STATE_CHANGE_FOCUS;
}

inline
void window_close(Window* const window) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) window->platform_window;
    CloseWindow(platform_window->hwnd);
    DestroyWindow(platform_window->hwnd);
}

HBITMAP CreateBitmapFromRGBA(
    HDC const __restrict hdc,
    const byte* const __restrict rgba,
    int32 width, int32 height
) NO_EXCEPT
{
    BITMAPINFO bmi = {0};
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