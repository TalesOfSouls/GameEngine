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

#include "../../../stdlib/Types.h"
#include "../../../memory/BufferMemory.h"
#include "../../../math/matrix/Matrix.h"
#include "../../../camera/Camera.h"
#include "../SoftwareRenderer.h"
#include <windows.h>
#include <wingdi.h>

// Used for initialization and resize
inline
void soft_renderer_update(
    SoftwareRenderer* __restrict renderer,
    const Window* __restrict w,
    size_t vram
) {
    if ((renderer->dimension.width == w->width
        && renderer->dimension.height == w->height)
        || w->width == 0 || w->height == 0
    ) {
        return;
    }

    if (renderer->platform.dib) {
        DeleteObject(renderer->platform.dib);
        renderer->platform.dib = NULL;
    }

    if (renderer->platform.hdc) {
        DeleteDC(renderer->platform.hdc);
        renderer->platform.hdc = NULL;
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

    renderer->dimension.width = w->width;
    renderer->dimension.height = w->height;

    renderer->platform.hwnd = w->hwnd;
    renderer->platform.window_hdc = w->hdc;

    renderer->platform.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    renderer->platform.bmi.bmiHeader.biWidth = renderer->dimension.width;
    renderer->platform.bmi.bmiHeader.biHeight = -renderer->dimension.height; // top-down
    renderer->platform.bmi.bmiHeader.biPlanes = 1;
    renderer->platform.bmi.bmiHeader.biBitCount = 32;
    renderer->platform.bmi.bmiHeader.biCompression = BI_RGB;

    renderer->platform.dib = CreateDIBSection(w->hdc, &renderer->platform.bmi, DIB_RGB_COLORS, (void **) &renderer->pixels, NULL, 0);
    ASSERT_TRUE(renderer->platform.dib && renderer->pixels);

    renderer->platform.hdc = CreateCompatibleDC(w->hdc);
    SelectObject(renderer->platform.hdc, renderer->platform.dib);

    if (renderer->max_dimension.height * renderer->max_dimension.width < renderer->dimension.width * renderer->dimension.height) {
        if (renderer->zbuffer) {
            platform_aligned_free((void **) &renderer->zbuffer);
        }

        renderer->zbuffer = (f32 *) platform_alloc_aligned(renderer->dimension.width * renderer->dimension.height * sizeof(f32), sizeof(size_t));
        renderer->max_dimension.width = renderer->dimension.width;
        renderer->max_dimension.height = renderer->dimension.height;
    }
}

inline
void soft_buffer_swap(SoftwareRenderer* __restrict renderer) NO_EXCEPT
{
    // @bug We probably have to replace window_hdc with GetDC(renderer->hwnd)
    // Yes opengl doesn't need that but software rendering needs it
    // This is comparable with SwapBuffer()
    BitBlt(
        renderer->platform.window_hdc,
        0, 0, renderer->dimension.width, renderer->dimension.height,
        renderer->platform.hdc,
        0, 0, SRCCOPY
    );
}

#endif