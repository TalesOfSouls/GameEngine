/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_WINDOW_C
#define COMS_PLATFORM_WIN32_WINDOW_C

#include "../../stdlib/Stdlib.h"
#include "../../system/Window.h"
#include "../../utils/StringUtils.h"
#include <windows.h>

FORCE_INLINE
v2_uint16 monitor_dimensions_get()
{
    return {(uint16) GetSystemMetrics(SM_CXSCREEN), (uint16) GetSystemMetrics(SM_CYSCREEN)};
}

FORCE_INLINE
uint32 window_dpi_get(const Window* const w) NO_EXCEPT
{
    HWND hwnd = ((WindowPlatform*) w->platform_window)->hwnd;

    uint32 dpi = GetDpiForWindow(hwnd);
    if (!dpi) {
        dpi = 96;
    }

    return dpi;
}

FORCE_INLINE
void physical_resolution_update(Window* const w) {
    WindowPlatform* const platform_window = (WindowPlatform*) w->platform_window;

    const uint32 dpi = GetDpiForWindow(platform_window->hwnd);

    w->dpi = (byte)(dpi ? dpi : 96);
    w->state_current.physical_width = w->state_current.client_width;
    w->state_current.physical_height = w->state_current.client_height;
}

FORCE_INLINE
void monitor_resolution(const Window* __restrict w, v2_int32* __restrict resolution) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*)w->platform_window;
    HWND hwnd = platform_window->hwnd;
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitor_info = {sizeof(MONITORINFO)};

    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    resolution->width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    resolution->height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
}

FORCE_INLINE
void monitor_resolution(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*)w->platform_window;
    HWND hwnd = platform_window->hwnd;
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitor_info = {sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    w->state_current.logical_width = (uint16) (monitor_info.rcMonitor.right - monitor_info.rcMonitor.left);
    w->state_current.logical_height = (uint16) (monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top);

    physical_resolution_update(w);
}

bool monitor_dim_get(
    const Window* const w,
    v4_int32* const dimension
) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*)w->platform_window;

    HMONITOR monitor = MonitorFromWindow(
        platform_window->hwnd,
        MONITOR_DEFAULTTONEAREST
    );

    MONITORINFO monitor_info = {sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        return false;
    }

    dimension->x = monitor_info.rcMonitor.left;
    dimension->y = monitor_info.rcMonitor.top;
    dimension->width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    dimension->height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

    return true;
}

inline
void window_resolution_update(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*) w->platform_window;

    HWND hwnd = platform_window->hwnd;

    // Get window dimensions
    RECT window_rect;
    if (!GetWindowRect(hwnd, &window_rect)) {
        return;
    }

    w->state_current.x = (uint16) window_rect.left;
    w->state_current.y = (uint16) window_rect.top;
    w->state_current.logical_width = (uint16) (window_rect.right - window_rect.left);
    w->state_current.logical_height = (uint16) (window_rect.bottom - window_rect.top);

    // Get client dimensions (window without border, title etc. = drawable area)
    RECT client_rect;
    if (!GetClientRect(hwnd, &client_rect)) {
        return;
    }

    POINT client_origin = {0, 0};
    if (!ClientToScreen(hwnd, &client_origin)) {
        return;
    }

    // @question We don't have client x/y, do we need it?
    //w->state_current.x = (uint16) client_origin.x;
    //w->state_current.y = (uint16) client_origin.y;

    w->state_current.client_width = (uint16)(client_rect.right - client_rect.left);
    w->state_current.client_height = (uint16)(client_rect.bottom - client_rect.top);

    physical_resolution_update(w);

    if (!w->state_current.client_width
        || !w->state_current.client_height
    ) {
        w->state_flag |= WINDOW_STATE_FLAG_DIMENSIONLESS;
    } else {
        w->state_flag &= ~WINDOW_STATE_FLAG_DIMENSIONLESS;
    }
}

inline
void window_fullscreen_apply(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*)w->platform_window;

    HWND hwnd = platform_window->hwnd;
    v4_int32 monitor_dim;
    if (!monitor_dim_get(w, &monitor_dim)) {
        return;
    }

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowPos(
        hwnd,
        HWND_TOP,
        monitor_dim.x,
        monitor_dim.y,
        monitor_dim.width,
        monitor_dim.height,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED
    );

    w->state_current.x = (uint16) monitor_dim.x;
    w->state_current.y = (uint16) monitor_dim.y;

    w->state_current.client_width = (uint16) monitor_dim.width;
    w->state_current.client_height = (uint16) monitor_dim.height;

    w->state_current.logical_width = w->state_current.client_width;
    w->state_current.logical_height = w->state_current.client_height;

    physical_resolution_update(w);
}

/**
 * Removes all window styles effectively turning it into a full screen window
 */
FORCE_INLINE
void window_style_remove(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*) w->platform_window;
    HWND hwnd = platform_window->hwnd;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);

    style &= ~WS_OVERLAPPEDWINDOW;
    style |= WS_POPUP;
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);

    window_resolution_update(w);

    // @question Do I need SetWindowPos here with SWP_FRAMECHANGED?
}

/**
 * Re-add window styles
 */
FORCE_INLINE
void window_style_add(Window* const w) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;

    LONG_PTR style = GetWindowLongPtrW(platform_window->hwnd, GWL_STYLE);
    style |= WS_OVERLAPPEDWINDOW;
    SetWindowLongPtr(platform_window->hwnd, GWL_STYLE, style);

    window_resolution_update(w);
}

inline
void window_restore(Window* const w) NO_EXCEPT
{
    window_state_restore(w);

    WindowPlatform* const platform_window = (WindowPlatform *) w->platform_window;

    SetWindowLongPtr(platform_window->hwnd, GWL_STYLE, w->state_old.style);
    SetWindowPos(
        platform_window->hwnd, HWND_TOP,
        w->state_old.x, w->state_old.y,
        w->state_old.logical_width, w->state_old.logical_height,
        SWP_NOACTIVATE | SWP_NOZORDER
    );

    window_resolution_update(w);
}

static
bool window_client_screen_rect_get(
    const Window* const w,
    RECT* const rect
) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform*)w->platform_window;
    HWND hwnd = platform_window->hwnd;

    RECT client_rect;
    if (!GetClientRect(hwnd, &client_rect)) {
        return false;
    }

    POINT top_left = {client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd, &top_left)) {
        return false;
    }

    POINT bottom_right = {client_rect.right, client_rect.bottom};
    if (!ClientToScreen(hwnd, &bottom_right)) {
        return false;
    }

    rect->left = top_left.x;
    rect->top = top_left.y;
    rect->right = bottom_right.x;
    rect->bottom = bottom_right.y;

    return true;
}

FORCE_INLINE
void window_mouse_clip(Window* const w) NO_EXCEPT
{
    RECT rect;
    if (window_client_screen_rect_get(w, &rect)) {
        ClipCursor(&rect);
    }
}

FORCE_INLINE
void window_state_backup(Window* const w) NO_EXCEPT
{
    memcpy(&w->state_old, &w->state_current, sizeof(w->state_current));
    w->state_old.style = GetWindowLongPtrW(
        ((WindowPlatform *) w->platform_window)->hwnd,
        GWL_STYLE
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
        SMN(cbSize) sizeof(WNDCLASSEXW),
        SMN(style) CS_OWNDC,
        SMN(lpfnWndProc) wndproc,
        SMN(cbClsExtra) 0,
        SMN(cbWndExtra) 0,
        SMN(hInstance) platform_window->hInstance,
        SMN(hIcon) NULL,
        SMN(hCursor) LoadCursor(NULL, IDC_ARROW),
        SMN(hbrBackground) NULL,
        SMN(lpszMenuName) NULL,
        SMN(lpszClassName) (LPCWSTR) title,
    };

    if (!RegisterClassExW(&wc)) {
        return;
    }

    HDC hdc = GetDC(NULL);
    int dpi = 96;
    if (hdc) {
        dpi = (int) GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }

    RECT rect = {
        0,
        0,
        (LONG) window->state_current.logical_width,
        (LONG) window->state_current.logical_height
    };

    AdjustWindowRectExForDpi(
        &rect,
        WS_OVERLAPPEDWINDOW,
        FALSE,
        0,
        dpi
    );

    const int32 outer_width = rect.right - rect.left;
    const int32 outer_height = rect.bottom - rect.top;

    platform_window->hwnd = CreateWindowExW((DWORD) NULL,
        wc.lpszClassName, NULL,
        WS_OVERLAPPEDWINDOW,
        window->state_current.x, window->state_current.y,
        outer_width,
        outer_height,
        NULL, NULL, platform_window->hInstance, window
    );

    window_resolution_update(window);

    if (window->state_flag & WINDOW_STATE_FLAG_FULLSCREEN) {
        window_state_backup(window);
        window_fullscreen_apply(window);
        window->state_flag |= WINDOW_STATE_FLAG_FULLSCREEN;
        window_resolution_update(window);
    }

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
void window_hide(Window* const window) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) window->platform_window;
    ShowWindow(platform_window->hwnd, SW_HIDE);
}

inline
void window_close(Window* const window) NO_EXCEPT
{
    WindowPlatform* const platform_window = (WindowPlatform *) window->platform_window;
    PostMessageW(platform_window->hwnd, WM_CLOSE, 0, 0);
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