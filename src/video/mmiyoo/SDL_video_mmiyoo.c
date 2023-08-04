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

#if SDL_VIDEO_DRIVER_MMIYOO

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>

#include "../SDL_sysvideo.h"
#include "SDL_version.h"
#include "SDL_syswm.h"
#include "SDL_loadso.h"
#include "SDL_events.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_video_mmiyoo.h"
#include "SDL_event_mmiyoo.h"
#include "SDL_framebuffer_mmiyoo.h"
#include "SDL_opengles_mmiyoo.h"

#define MMIYOO_DRIVER_NAME "mmiyoo"
#define FB_W    640
#define FB_H    480
#define FB_BPP  4
#define FB_SIZE (FB_W * FB_H * FB_BPP * 2)

MMIYOO_VideoInfo MMiyooVideoInfo={0};

extern MMIYOO_EventInfo MMiyooEventInfo;

static int fb_idx = 0;
static int fb_dev = -1;
static void *fb_vaddr[2] = {0};
static uint32_t *fb_mem = NULL;
static struct fb_var_screeninfo vinfo = {0};

static int MMIYOO_VideoInit(_THIS);
static int MMIYOO_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
static void MMIYOO_VideoQuit(_THIS);

static int fb_init(void)
{
    fb_idx = 0;
    fb_dev = open("/dev/fb0", O_RDWR);
    if(fb_dev < 0) {
        printf("failed to open /dev/fb0\n");
        return -1;
    }

    if(ioctl(fb_dev, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        close(fb_dev);
        printf("failed to ioctl /dev/fb0\n");
        return -1;
    }
    vinfo.yres_virtual = vinfo.yres * 2;
    ioctl(fb_dev, FBIOPUT_VSCREENINFO, &vinfo);

    fb_mem = (uint32_t *)mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fb_dev, 0);
    if(fb_mem == (void *) -1) {
        close(fb_dev);
        printf("failed to mmap /dev/fb0\n");
        return -1;
    }
    memset(fb_mem, 0, FB_SIZE);
    fb_vaddr[0] = fb_mem;
    fb_vaddr[1] = ((uint8_t *)fb_mem) + (FB_W * FB_H * FB_BPP);
    return 0;
}

static int fb_deinit(void)
{
    munmap(fb_mem, FB_SIZE);
    fb_mem = NULL;

    close(fb_dev);
    fb_dev = -1;
    return 0;
}

void fb_fill(const void *pixels, const SDL_Rect *rect, int pitch)
{
    int x = 0, y = 0;
    uint32_t *d = fb_vaddr[fb_idx % 2];

    //printf("%s, x:%d, y:%d, w:%d, h:%d, pitch:%d\n", __func__, rect->x, rect->y, rect->w, rect->h, pitch);
    if((pitch == 512) && (MMiyooVideoInfo.window->w == 256) && (MMiyooVideoInfo.window->h == 384)) {
        uint32_t r=0, g=0, b=0, t=0;
        uint16_t *s = (uint16_t *)pixels;

        for(y = rect->y; y < rect->h; y++) {
            for(x = rect->x; x < rect->w; x++) {
                t = s[x + (y * rect->w)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;

                t = 0xff000000 | r | g | b;
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 1))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 1))] = t;
            }
        }
    }
    else if((pitch == 640) && (MMiyooVideoInfo.window->w == 320) && (MMiyooVideoInfo.window->h == 240)) {
        uint32_t r=0, g=0, b=0, t=0;
        uint16_t *s = (uint16_t *)pixels;

        for(y = rect->y; y < rect->h; y++) {
            for(x = rect->x; x < rect->w; x++) {
                t = s[x + (y * 320)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;

                t = 0xff000000 | r | g | b;
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 1))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 1))] = t;
            }
        }
    }
    else if((pitch == 1280) && (MMiyooVideoInfo.window->w == 320) && (MMiyooVideoInfo.window->h == 240)) {
        uint32_t t=0;
        uint32_t *s = (uint32_t *)pixels;

        for(y = rect->y; y < rect->h; y++) {
            for(x = rect->x; x < rect->w; x++) {
                t = 0xff000000 | s[x + (y * 320)];
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 0)) * 640) + (639 - ((x << 1) + 1))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 0))] = t;
                d[((479 - ((y << 1) + 1)) * 640) + (639 - ((x << 1) + 1))] = t;
            }
        }
    }
    else if((pitch == 960) && (MMiyooVideoInfo.window->w == 480) && (MMiyooVideoInfo.window->h == 272)) {
        uint32_t r=0, g=0, b=0, t=0, x0=0, y0=0;
        uint16_t *s = (uint16_t *)pixels;

        y0 = 59;
        for(y = rect->y; y < rect->h; y++, y0++) {
            x0 = 0;
            for(x = rect->x; x < rect->w; x++, x0++) {
                t = s[x + ((y + 0) * 480)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;
                
                t = 0xff000000 | r | g | b;
                d[((479 - y0) * 640) + (639 - x0)] = t;
                if(y && (y % 3) == 0){
                    d[((479 - (y0 + 1)) * 640) + (639 - x0)] = t;
                    if(x && (x % 3) == 0){
                        d[((479 - (y0 + 1)) * 640) + (639 - (x0 + 1))] = t;
                    }
                }
                if(x && (x % 3) == 0){
                    d[((479 - y0) * 640) + (639 - (x0 + 1))] = t;
                    x0+= 1;
                }
            }
            if(y && (y % 3) == 0){
                y0+= 1;
            }
        }
    }
    else if((pitch == 1920) && (MMiyooVideoInfo.window->w == 480) && (MMiyooVideoInfo.window->h == 272)) {
        uint32_t t=0, x0=0, y0=0;
        uint32_t *s = (uint32_t *)pixels;

        y0 = 59;
        for(y = rect->y; y < rect->h; y++, y0++) {
            x0 = 0;
            for(x = rect->x; x < rect->w; x++, x0++) {
                t = 0xff000000 | s[x + ((y + 0) * 480)];
                d[((479 - y0) * 640) + (639 - x0)] = t;
                if(y && (y % 3) == 0){
                    d[((479 - (y0 + 1)) * 640) + (639 - x0)] = t;
                    if(x && (x % 3) == 0){
                        d[((479 - (y0 + 1)) * 640) + (639 - (x0 + 1))] = t;
                    }
                }
                if(x && (x % 3) == 0){
                    d[((479 - y0) * 640) + (639 - (x0 + 1))] = t;
                    x0+= 1;
                }
            }
            if(y && (y % 3) == 0){
                y0+= 1;
            }
        }
    }
    else if((pitch == 960) && (MMiyooVideoInfo.window->w == 480) && (MMiyooVideoInfo.window->h == 360)) {
        uint32_t r=0, g=0, b=0, t=0, x0=0, y0=0;
        uint16_t *s = (uint16_t *)pixels;

        y0 = 0;
        for(y = rect->y; y < rect->h; y++, y0++) {
            x0 = 0;
            for(x = rect->x; x < rect->w; x++, x0++) {
                t = s[x + ((y + 0) * 480)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;
                
                t = 0xff000000 | r | g | b;
                d[((479 - y0) * 640) + (639 - x0)] = t;
                if(y && (y % 3) == 0){
                    d[((479 - (y0 + 1)) * 640) + (639 - x0)] = t;
                    if(x && (x % 3) == 0){
                        d[((479 - (y0 + 1)) * 640) + (639 - (x0 + 1))] = t;
                    }
                }
                if(x && (x % 3) == 0){
                    d[((479 - y0) * 640) + (639 - (x0 + 1))] = t;
                    x0+= 1;
                }
            }
            if(y && (y % 3) == 0){
                y0+= 1;
            }
        }
    }
    else if((pitch == 1920) && (MMiyooVideoInfo.window->w == 480) && (MMiyooVideoInfo.window->h == 360)) {
        uint32_t t=0, x0=0, y0=0;
        uint16_t *s = (uint16_t *)pixels;

        y0 = 0;
        for(y = rect->y; y < rect->h; y++, y0++) {
            x0 = 0;
            for(x = rect->x; x < rect->w; x++, x0++) {
                t = 0xff000000 | s[x + ((y + 0) * 480)];
                d[((479 - y0) * 640) + (639 - x0)] = t;
                if(y && (y % 3) == 0){
                    d[((479 - (y0 + 1)) * 640) + (639 - x0)] = t;
                    if(x && (x % 3) == 0){
                        d[((479 - (y0 + 1)) * 640) + (639 - (x0 + 1))] = t;
                    }
                }
                if(x && (x % 3) == 0){
                    d[((479 - y0) * 640) + (639 - (x0 + 1))] = t;
                    x0+= 1;
                }
            }
            if(y && (y % 3) == 0){
                y0+= 1;
            }
        }
    }
    else if((pitch == 1280) && (MMiyooVideoInfo.window->w == 640) && (MMiyooVideoInfo.window->h == 480)){
        uint32_t r, g, b, t;
        uint16_t *s = (uint16_t *)pixels;

        for(y = rect->y; y < rect->h; y++) {
            for(x = rect->x; x < rect->w; x++) {
                t = s[x + (y * 640)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;
                d[((479 - y) * 640) + (639 - x)] = (0xff000000 | r | g | b);
            }
        }
    }
    else if((pitch == 2560) && (MMiyooVideoInfo.window->w == 640) && (MMiyooVideoInfo.window->h == 480)){
        uint32_t *s = (uint32_t *)pixels;

        for(y = rect->y; y < rect->h; y++) {
            for(x = rect->x; x < rect->w; x++) {
                d[((479 - y) * 640) + (639 - x)] = 0xff000000 | s[x + (y * 640)];
            }
        }
    }
    else if((pitch == 1600) && (MMiyooVideoInfo.window->w == 800) && (MMiyooVideoInfo.window->h == 600)) {
        uint16_t *s = (uint16_t *)pixels;
        uint32_t r=0, g=0, b=0, t=0, xinc = 0, yinc = 0;

        yinc = 0;
        for(y = rect->y; y < rect->h; y++) {
            if(y && (y % 5) == 0) {
                yinc += 1;
            }
            
            xinc = 0;
            for(x = rect->x; x < rect->w; x++) {
                if(x && (x % 5) == 0) {
                    xinc += 1;
                }
                t = s[x + (y * 800)];
                r = (t & 0xf800) << 8;
                g = (t & 0x7e0) << 5;
                b = (t & 0x1f) << 3;
                d[((479 - (y - yinc)) * 640) + (639 - (x - xinc))] = (0xff000000 | r | g | b);
            }
        }
    }
    else if((pitch == 3200) && (MMiyooVideoInfo.window->w == 800) && (MMiyooVideoInfo.window->h == 600)) {
        uint32_t *s = (uint32_t *)pixels;
        uint32_t xinc = 0, yinc = 0;

        yinc = 0;
        for(y = rect->y; y < rect->h; y++) {
            if(y && (y % 5) == 0) {
                yinc += 1;
            }
            
            xinc = 0;
            for(x = rect->x; x < rect->w; x++) {
                if(x && (x % 5) == 0) {
                    xinc += 1;
                }
                d[((479 - (y - yinc)) * 640) + (639 - (x - xinc))] = s[x + (y * 800)];
            }
        }
    }

    if(MMiyooEventInfo.mode == MMIYOO_MOUSE_MODE) {
        int cc=0;
        int xpos = (MMiyooEventInfo.mouse.x * 640) / MMiyooEventInfo.mouse.xmax;
        int ypos = (MMiyooEventInfo.mouse.y * 480) / MMiyooEventInfo.mouse.ymax;
        uint32_t col[]={0xffff0000, 0xff00ff00, 0xff0000ff};
        for(y = -4; y < 5; y++) {
            for(x = -4; x < 5; x++) {
                d[((479 - (ypos + y)) * 640) + (639 - (xpos + x))] = col[cc / 3];
            }
            cc+= 1;
        }
    }
}

void fb_flip(void)
{
    int ret = 0;

    vinfo.yoffset = (fb_idx % 2) * vinfo.yres;
    ioctl(fb_dev, FBIOPAN_DISPLAY, &vinfo);
    ioctl(fb_dev, FBIO_WAITFORVSYNC, &ret);
    fb_idx += 1;
}

static int MMIYOO_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if((envr) && (SDL_strcmp(envr, MMIYOO_DRIVER_NAME) == 0)) {
        return 1;
    }
    return 0;
}

static void MMIYOO_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

int MMIYOO_CreateWindow(_THIS, SDL_Window *window)
{
    SDL_SetMouseFocus(window);
    MMiyooVideoInfo.window = window;
    MMiyooEventInfo.mouse.xmax = window->w;
    MMiyooEventInfo.mouse.ymax = window->h;
    MMiyooEventInfo.mouse.x = window->w / 2;
    MMiyooEventInfo.mouse.y = window->h / 2;
    printf("%s, w:%d, h:%d\n", __func__, window->w, window->h);

    printf("%s, fb_flip:%p\n", __func__, fb_flip);
    printf("%s, fb_idx:%p\n", __func__, &fb_idx);
    printf("%s, fb_vaddr:%p\n", __func__, fb_vaddr);
    printf("%s, fb_vaddr[0]:%p\n", __func__, fb_vaddr[0]);
    printf("%s, fb_vaddr[1]:%p\n", __func__, fb_vaddr[1]);
    glUpdateBufferSettings(fb_flip, &fb_idx, fb_vaddr);
    return 0;
}

int MMIYOO_CreateWindowFrom(_THIS, SDL_Window *window, const void *data)
{
    return SDL_Unsupported();
}

static SDL_VideoDevice *MMIYOO_CreateDevice(int devindex)
{
    SDL_VideoDevice *device=NULL;
    SDL_GLDriverData *gldata=NULL;

    if(!MMIYOO_Available()) {
        return (0);
    }

    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if(!device) {
        SDL_OutOfMemory();
        return (0);
    }
    device->is_dummy = SDL_TRUE;

    device->VideoInit = MMIYOO_VideoInit;
    device->VideoQuit = MMIYOO_VideoQuit;
    device->SetDisplayMode = MMIYOO_SetDisplayMode;
    device->PumpEvents = MMIYOO_PumpEvents;
    device->CreateSDLWindow = MMIYOO_CreateWindow;
    device->CreateSDLWindowFrom = MMIYOO_CreateWindowFrom;
    device->CreateWindowFramebuffer = MMIYOO_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = MMIYOO_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = MMIYOO_DestroyWindowFramebuffer;

    device->GL_LoadLibrary = glLoadLibrary;
    device->GL_GetProcAddress = glGetProcAddress;
    device->GL_CreateContext = glCreateContext;
    device->GL_SetSwapInterval = glSetSwapInterval;
    device->GL_SwapWindow = glSwapWindow;
    device->GL_MakeCurrent = glMakeCurrent;
    device->GL_DeleteContext = glDeleteContext;
    device->GL_UnloadLibrary = glUnloadLibrary;
    
    gldata = (SDL_GLDriverData*)SDL_calloc(1, sizeof(SDL_GLDriverData));
    if(gldata == NULL) {
        SDL_OutOfMemory();
        SDL_free(device);
        return NULL;
    }
    device->gl_data = gldata;

    device->free = MMIYOO_DeleteDevice;
    return device;
}

VideoBootStrap MMIYOO_bootstrap = {MMIYOO_DRIVER_NAME, "MMIYOO VIDEO DRIVER", MMIYOO_CreateDevice};

int MMIYOO_VideoInit(_THIS)
{
    SDL_DisplayMode mode={0};
    SDL_VideoDisplay display={0};

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 640;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 640;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);
    
    SDL_AddVideoDisplay(&display, SDL_FALSE);
    
    fb_init();
    MMIYOO_EventInit();
    return 0;
}

static int MMIYOO_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0;
}

void MMIYOO_VideoQuit(_THIS)
{
    fb_deinit();
    MMIYOO_EventDeinit();
}

#endif

