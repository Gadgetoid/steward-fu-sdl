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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "../../events/SDL_events_c.h"
#include "../../core/linux/SDL_evdev.h"
#include "../../thread/SDL_systhread.h"

#include "SDL_video_mmiyoo.h"
#include "SDL_event_mmiyoo.h"

MMIYOO_EventInfo MMiyooEventInfo={0};
extern MMIYOO_VideoInfo MMiyooVideoInfo;

static int event_fd=-1;
static int running = 0;
static SDL_sem *event_sem = NULL;
static SDL_Thread *thread = NULL;
static uint32_t pre_keypad_bitmaps=0;

int EventUpdate(void *data)
{
    uint32_t bit=0;
    struct input_event ev = {0};

    while(running) {
        SDL_SemWait(event_sem);
        if(event_fd > 0) {
            if(read(event_fd, &ev, sizeof(struct input_event))) {
                if((ev.type == EV_KEY) && (ev.value != 2)){
                    switch(ev.code) {
                    case 103: bit = (1 << MYKEY_UP);     break;
                    case 108: bit = (1 << MYKEY_DOWN);   break;
                    case 105: bit = (1 << MYKEY_LEFT);   break;
                    case 106: bit = (1 << MYKEY_RIGHT);  break;
                    case 57:  bit = (1 << MYKEY_A);      break;
                    case 29:  bit = (1 << MYKEY_B);      break;
                    case 42:  bit = (1 << MYKEY_X);      break;
                    case 56:  bit = (1 << MYKEY_Y);      break;
                    case 28:  bit = (1 << MYKEY_START);  break;
                    case 97:  bit = (1 << MYKEY_SELECT); break;
                    case 1:   bit = (1 << MYKEY_MENU);   break;
                    case 18:  bit = (1 << MYKEY_L1);     break;
                    case 15:  bit = (1 << MYKEY_L2);     break;
                    case 20:  bit = (1 << MYKEY_R1);     break;
                    case 14:  bit = (1 << MYKEY_R2);     break;
                    case 116: bit = (1 << MYKEY_POWER);  break;
                    }

                    if(bit){
                        if(ev.value){
                            //printf("%s, %d(1)\n", __func__, bit);
                            MMiyooEventInfo.keypad.bitmaps|= bit;
                        }
                        else{
                            //printf("%s, %d(0)\n", __func__, bit);
                            MMiyooEventInfo.keypad.bitmaps&= ~bit;
                        }
                    }

                    if((MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_L2)) && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_L2))){
                        MMiyooEventInfo.mode = (MMiyooEventInfo.mode == MMIYOO_KEYPAD_MODE) ? MMIYOO_MOUSE_MODE : MMIYOO_KEYPAD_MODE;
                        //printf("%s, event mode: %d\n", __func__, MMiyooEventInfo.mode);
                    }
                }
            }
        }
        SDL_SemPost(event_sem);
        usleep(1000000 / 60);
    }
    return 0;
}

void MMIYOO_EventInit(void)
{
    pre_keypad_bitmaps = 0;
    memset(&MMiyooEventInfo, 0, sizeof(MMiyooEventInfo));
    MMiyooEventInfo.mouse.x = 9;
    MMiyooEventInfo.mouse.y = 9;
    MMiyooEventInfo.mouse.xmax = 640;
    MMiyooEventInfo.mouse.ymax = 480;
    MMiyooEventInfo.mode = MMIYOO_KEYPAD_MODE;

#if defined(MMIYOO)
    event_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(event_fd < 0){
        printf("failed to open /dev/input/event0\n");
    }
#endif

    if((event_sem =  SDL_CreateSemaphore(1)) == NULL) {
        SDL_SetError("Can't create input semaphore");
        return;
    }

    running = 1;
    if((thread = SDL_CreateThreadInternal(EventUpdate, "MMIYOOInputThread", 4096, NULL)) == NULL) {
        SDL_SetError("Can't create input thread");
        return;
    }
}

void MMIYOO_EventDeinit(void)
{
    running = 0;
    SDL_WaitThread(thread, NULL);
    SDL_DestroySemaphore(event_sem);
    if(event_fd > 0) {
        close(event_fd);
        event_fd = -1;
    }
}

void MMIYOO_PumpEvents(_THIS)
{
    const SDL_Scancode code[]={
        SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT,
        SDLK_SPACE, SDLK_LCTRL, SDLK_LSHIFT, SDLK_LALT,
        SDLK_e, SDLK_t, SDLK_TAB, SDLK_BACKSPACE,
        SDLK_RCTRL, SDLK_RETURN, SDLK_ESCAPE
    };
    /*const char *name[]={
        "SDLK_UP", "SDLK_DOWN", "SDLK_LEFT", "SDLK_RIGHT",
        "SDLK_SPACE", "SDLK_LCTRL", "SDLK_LSHIFT", "SDLK_LALT",
        "SDLK_e", "SDLK_t", "SDLK_TAB", "SDLK_BACKSPACE",
        "SDLK_RCTRL", "SDLK_RETURN", "SDLK_ESCAPE"
    };*/

    SDL_SemWait(event_sem);
    if(MMiyooEventInfo.mode == MMIYOO_KEYPAD_MODE){
        if(pre_keypad_bitmaps != MMiyooEventInfo.keypad.bitmaps){
            int cc=0;
            uint32_t v0=pre_keypad_bitmaps;
            uint32_t v1=MMiyooEventInfo.keypad.bitmaps;
            for(cc=0; cc<15; cc++){
                if((v0 & 1) != (v1 & 1)){
                    //printf("%s, SendKeyboardKey: %s(%d)\n", __func__, name[cc], (v1 & 1));
                    SDL_SendKeyboardKey((v1 & 1) ? SDL_PRESSED : SDL_RELEASED, SDL_GetScancodeFromKey(code[cc]));
                }
                v0>>= 1;
                v1>>= 1;
            }
            pre_keypad_bitmaps = MMiyooEventInfo.keypad.bitmaps;
        }
    }
    else{
        int updated=0;

        if(pre_keypad_bitmaps != MMiyooEventInfo.keypad.bitmaps){
            uint32_t v0=pre_keypad_bitmaps;
            uint32_t v1=MMiyooEventInfo.keypad.bitmaps;

            if((v0 & (1 << MYKEY_Y)) != (v1 & (1 << MYKEY_Y))){
                //printf("%s, SDL_BUTTON_LEFT: %d\n", __func__, (v1 & (1 << MYKEY_Y)));
                SDL_SendMouseButton(MMiyooVideoInfo.window, 0, (v1 & (1 << MYKEY_Y)) ? SDL_PRESSED : SDL_RELEASED, SDL_BUTTON_LEFT);
            }
            if((v0 & (1 << MYKEY_A)) != (v1 & (1 << MYKEY_A))){
                //printf("%s, SDL_BUTTON_RIGHT: %d\n", __func__, (v1 & (1 << MYKEY_A)));
                SDL_SendMouseButton(MMiyooVideoInfo.window, 0, (v1 & (1 << MYKEY_A)) ? SDL_PRESSED : SDL_RELEASED, SDL_BUTTON_RIGHT);
            }
        }

        if(MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_UP)){
            if(MMiyooEventInfo.mouse.y > 15){
                updated = 1;
                MMiyooEventInfo.mouse.y-= 9;
            }
        }
        else if(MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_DOWN)){
            if(MMiyooEventInfo.mouse.y < (MMiyooEventInfo.mouse.ymax - 15)){
                updated = 1;
                MMiyooEventInfo.mouse.y+= 9;
            }
        }
        else if(MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_LEFT)){
            if(MMiyooEventInfo.mouse.x > 15){
                updated = 1;
                MMiyooEventInfo.mouse.x-= 9;
            }
        }
        else if(MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_RIGHT)){
            if(MMiyooEventInfo.mouse.x < (MMiyooEventInfo.mouse.xmax - 15)){
                updated = 1;
                MMiyooEventInfo.mouse.x+= 9;
            }
        }

        if(updated){
            //printf("%s, SendMouseMotion: x:%d, y:%d\n", __func__, MMiyooEventInfo.mouse.x, MMiyooEventInfo.mouse.y);
            SDL_SendMouseMotion(MMiyooVideoInfo.window, 0, 0, MMiyooEventInfo.mouse.x, MMiyooEventInfo.mouse.y);
        }
        pre_keypad_bitmaps = MMiyooEventInfo.keypad.bitmaps;
    }
    SDL_SemPost(event_sem);
}

#endif

