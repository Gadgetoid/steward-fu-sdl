/*
  Customized version for Miyoo-Mini handheld.
  Only tested under Miyoo-Mini stock OS (original firmware) with Parasyte compatible layer.

  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2022-2022 Steward Fu <steward.fu@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_RENDER_MMIYOO

#include "SDL_hints.h"
#include "../SDL_sysrender.h"

typedef struct MMIYOO_TextureData {
    void *data;
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int bits;
    unsigned int format;
    unsigned int pitch;
} MMIYOO_TextureData;

typedef struct {
    SDL_Texture *boundTarget;
    SDL_bool initialized;
    unsigned int bpp;
    SDL_bool vsync;
} MMIYOO_RenderData;

void fb_flip(void);
void fb_fill(const void *pixels, const SDL_Rect *rect, int pitch);

static void MMIYOO_WindowEvent(SDL_Renderer *renderer, const SDL_WindowEvent *event)
{
}

static int MMIYOO_CreateTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    MMIYOO_TextureData *mmiyoo_texture = (MMIYOO_TextureData *)SDL_calloc(1, sizeof(*mmiyoo_texture));

    if(!mmiyoo_texture) {
        return SDL_OutOfMemory();
    }

    mmiyoo_texture->width = texture->w;
    mmiyoo_texture->height = texture->h;
    mmiyoo_texture->format = texture->format;

    switch(mmiyoo_texture->format) {
    case SDL_PIXELFORMAT_RGB565:
        mmiyoo_texture->bits = 16;
        break;
    case SDL_PIXELFORMAT_ARGB8888:
        mmiyoo_texture->bits = 32;
        break;
    default:
        printf("%s, invalid format %d\n", __func__, mmiyoo_texture->format);
        return -1;
    }

    mmiyoo_texture->pitch = mmiyoo_texture->width * SDL_BYTESPERPIXEL(texture->format);
    mmiyoo_texture->size = mmiyoo_texture->height * mmiyoo_texture->pitch;
    mmiyoo_texture->data = SDL_calloc(1, mmiyoo_texture->size);

    if(!mmiyoo_texture->data) {
        SDL_free(mmiyoo_texture);
        return SDL_OutOfMemory();
    }
    texture->driverdata = mmiyoo_texture;
    return 0;
}

static int MMIYOO_LockTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch)
{
    MMIYOO_TextureData *mmiyoo_texture = (MMIYOO_TextureData *)texture->driverdata;

    *pixels = (void *)((Uint8 *)mmiyoo_texture->data + rect->y * mmiyoo_texture->pitch + rect->x * SDL_BYTESPERPIXEL(texture->format));
    *pitch = mmiyoo_texture->pitch;
    return 0;
}

static int MMIYOO_UpdateTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch)
{
    Uint8 *dst = NULL;
    int dpitch = 0;

    MMIYOO_LockTexture(renderer, texture, rect, (void **)&dst, &dpitch);
    fb_fill(dst, rect, dpitch);
    fb_flip();
    return 0;
}

static void MMIYOO_UnlockTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
    SDL_Rect rect;
    MMIYOO_TextureData *mmiyoo_texture = (MMIYOO_TextureData *)texture->driverdata;

    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    MMIYOO_UpdateTexture(renderer, texture, &rect, mmiyoo_texture->data, mmiyoo_texture->pitch);
}

static void MMIYOO_SetTextureScaleMode(SDL_Renderer *renderer, SDL_Texture *texture, SDL_ScaleMode scaleMode)
{
}

static int MMIYOO_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture)
{
    return 0;
}

static int MMIYOO_QueueSetViewport(SDL_Renderer *renderer, SDL_RenderCommand *cmd)
{
    return 0;
}

static int MMIYOO_QueueDrawPoints(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FPoint *points, int count)
{
    return 0;
}

static int MMIYOO_QueueGeometry(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture,
                                const float *xy, int xy_stride, const SDL_Color *color, int color_stride, const float *uv, int uv_stride,
                                int num_vertices, const void *indices, int num_indices, int size_indices,
                                float scale_x, float scale_y)
{
    return 0;
}

static int MMIYOO_QueueFillRects(SDL_Renderer *renderer, SDL_RenderCommand *cmd, const SDL_FRect *rects, int count)
{
    return 0;
}

static int MMIYOO_QueueCopy(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_FRect *dstrect)
{
    return 0;
}

static int MMIYOO_QueueCopyEx(SDL_Renderer *renderer, SDL_RenderCommand *cmd, SDL_Texture *texture,
                              const SDL_Rect *srcrect, const SDL_FRect *dstrect,
                              const double angle, const SDL_FPoint *center, const SDL_RendererFlip flip)
{
    return 0;
}

static int MMIYOO_RunCommandQueue(SDL_Renderer *renderer, SDL_RenderCommand *cmd, void *vertices, size_t vertsize)
{
    return 0;
}

static int MMIYOO_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rect, Uint32 pixel_format, void *pixels, int pitch)
{
    return SDL_Unsupported();
}

static void MMIYOO_RenderPresent(SDL_Renderer *renderer)
{
}

static void MMIYOO_DestroyTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
}

static void MMIYOO_DestroyRenderer(SDL_Renderer *renderer)
{
    MMIYOO_RenderData *data = (MMIYOO_RenderData *)renderer->driverdata;

    if(data) {
        if(!data->initialized) {
            return;
        }

        data->initialized = SDL_FALSE;
        SDL_free(data);
    }
    SDL_free(renderer);
}

static int MMIYOO_SetVSync(SDL_Renderer *renderer, const int vsync)
{
    return 0;
}

SDL_Renderer *MMIYOO_CreateRenderer(SDL_Window *window, Uint32 flags)
{
    int pixelformat = 0;
    SDL_Renderer *renderer = NULL;
    MMIYOO_RenderData *data = NULL;

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if(!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (MMIYOO_RenderData *) SDL_calloc(1, sizeof(*data));
    if(!data) {
        MMIYOO_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }

    renderer->WindowEvent = MMIYOO_WindowEvent;
    renderer->CreateTexture = MMIYOO_CreateTexture;
    renderer->UpdateTexture = MMIYOO_UpdateTexture;
    renderer->LockTexture = MMIYOO_LockTexture;
    renderer->UnlockTexture = MMIYOO_UnlockTexture;
    renderer->SetTextureScaleMode = MMIYOO_SetTextureScaleMode;
    renderer->SetRenderTarget = MMIYOO_SetRenderTarget;
    renderer->QueueSetViewport = MMIYOO_QueueSetViewport;
    renderer->QueueSetDrawColor = MMIYOO_QueueSetViewport;
    renderer->QueueDrawPoints = MMIYOO_QueueDrawPoints;
    renderer->QueueDrawLines = MMIYOO_QueueDrawPoints;
    renderer->QueueGeometry = MMIYOO_QueueGeometry;
    renderer->QueueFillRects = MMIYOO_QueueFillRects;
    renderer->QueueCopy = MMIYOO_QueueCopy;
    renderer->QueueCopyEx = MMIYOO_QueueCopyEx;
    renderer->RunCommandQueue = MMIYOO_RunCommandQueue;
    renderer->RenderReadPixels = MMIYOO_RenderReadPixels;
    renderer->RenderPresent = MMIYOO_RenderPresent;
    renderer->DestroyTexture = MMIYOO_DestroyTexture;
    renderer->DestroyRenderer = MMIYOO_DestroyRenderer;
    renderer->SetVSync = MMIYOO_SetVSync;
    renderer->info = MMIYOO_RenderDriver.info;
    renderer->info.flags = (SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    renderer->driverdata = data;
    renderer->window = window;

    if(data->initialized != SDL_FALSE) {
        return 0;
    }
    data->initialized = SDL_TRUE;

    if(flags & SDL_RENDERER_PRESENTVSYNC) {
        data->vsync = SDL_TRUE;
    }
    else {
        data->vsync = SDL_FALSE;
    }

    pixelformat = SDL_GetWindowPixelFormat(window);
    switch(pixelformat) {
    case SDL_PIXELFORMAT_RGB565:
        data->bpp = 2;
        break;
    case SDL_PIXELFORMAT_ARGB8888:
        data->bpp = 4;
        break;
    }
    return renderer;
}

SDL_RenderDriver MMIYOO_RenderDriver = {
    .CreateRenderer = MMIYOO_CreateRenderer,
    .info = {
        .name = "MMIYOO",
        .flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE,
        .num_texture_formats = 2,
        .texture_formats = {
            [0] = SDL_PIXELFORMAT_RGB565, [2] = SDL_PIXELFORMAT_ARGB8888,
        },
        .max_texture_width = 800,
        .max_texture_height = 600,
    }
};

#endif

