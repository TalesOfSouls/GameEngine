/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_WIN32_SOFTWARE_RENDERER_C
#define COMS_GPUAPI_SOFTWARE_WIN32_SOFTWARE_RENDERER_C

#include "../../../stdlib/Stdlib.h"
#include "../../../memory/BufferMemory.h"
#include "../../../math/matrix/Matrix.h"
#include "../../../camera/Camera.h"
#include "../SoftwareRenderer.h"
#include <windows.h>
#include <wingdi.h>

// Used for initialization and resize
inline
void soft_renderer_update(
    SoftwareRenderer* const __restrict renderer,
    const Window* __restrict w,
    size_t vram
) NO_EXCEPT
{
    if ((renderer->dimension.width == w->physical_width
        && renderer->dimension.height == w->physical_height)
        || w->physical_width == 0 || w->physical_height == 0
    ) {
        return;
    }

    if (renderer->platform.dib) {
        DeleteObject(renderer->platform.dib);
        renderer->platform.dib = NULL;
    }

    if (renderer->platform.mem_hdc) {
        DeleteDC(renderer->platform.mem_hdc);
        renderer->platform.mem_hdc = NULL;
    }

    if (!renderer->buf.size) {
        chunk_alloc(&renderer->buf, (uint32) (vram / 64), 64, 64);
    }

    /*
    @bug How to handle?
    if (renderer->zbuffer) {
        free(renderer->zbuffer);
        renderer->zbuffer = NULL;
    }
    */

    renderer->dimension.width = w->physical_width;
    renderer->dimension.height = w->physical_height;

    renderer->platform.hwnd = w->hwnd;

    if (!renderer->platform.window_hdc) {
        renderer->platform.window_hdc = GetDC(w->hwnd);
    }

    renderer->platform.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    renderer->platform.bmi.bmiHeader.biWidth = renderer->dimension.width;
    renderer->platform.bmi.bmiHeader.biHeight = renderer->dimension.height; // down-up
    renderer->platform.bmi.bmiHeader.biPlanes = 1;
    renderer->platform.bmi.bmiHeader.biBitCount = 32;
    renderer->platform.bmi.bmiHeader.biCompression = BI_RGB;

    // This also reserves memory for renderer->pixels
    renderer->platform.dib = CreateDIBSection(
        renderer->platform.window_hdc,
        &renderer->platform.bmi,
        DIB_RGB_COLORS,
        (void **) &renderer->pixels,
        NULL, 0
    );
    ASSERT_TRUE(renderer->platform.dib && renderer->pixels);

    renderer->platform.mem_hdc = CreateCompatibleDC(renderer->platform.window_hdc);
    SelectObject(renderer->platform.mem_hdc, renderer->platform.dib);

    if (renderer->max_dimension.height * renderer->max_dimension.width < renderer->dimension.width * renderer->dimension.height) {
        if (renderer->zbuffer) {
            platform_aligned_free((void **) &renderer->zbuffer);
        }

        renderer->zbuffer = (f32 *) platform_alloc_aligned(renderer->dimension.width * renderer->dimension.height * sizeof(f32), sizeof(size_t) * 4);
        renderer->max_dimension.width = renderer->dimension.width;
        renderer->max_dimension.height = renderer->dimension.height;
        soft_clear(renderer);
    }
}

inline
void soft_buffer_swap(SoftwareRenderer* const __restrict renderer) NO_EXCEPT
{
    // This is comparable with SwapBuffer()
    BitBlt(
        renderer->platform.window_hdc,
        0, 0, renderer->dimension.width, renderer->dimension.height,
        renderer->platform.mem_hdc,
        0, 0, SRCCOPY
    );
}

#endif