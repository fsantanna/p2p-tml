#if 0
#!/bin/sh
gcc -g -Wall `sdl2-config --cflags` -DP2P_LATENCY=5 -DP2P_WAIT=0 p2p.c move2.c -o xmove2 `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image
exit
#endif

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_image.h>
#include "p2p.h"

enum {
    P2P_EVT_KEY = P2P_EVT_NEXT
};

void cb_ini (int);
void cb_sim (p2p_evt);
void cb_eff (int trv);
int  cb_rec (SDL_Event* sdl, p2p_evt* evt);

#define FPS   50
#define SIZE  400
#define VEL   2
#define DIM   10

int NET[21][10] = {
    //{  1, 20, -1, -1, -1, -1, -1, -1, -1, -1 }, // 0
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // 0
    {  0,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  4,  8, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4,  6, -1, -1, -1, -1, -1, -1, -1, -1 }, // 5
    {  5,  7,  9, -1, -1, -1, -1, -1, -1, -1 },
    {  6,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  6, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  9, 11, -1, -1, -1, -1, -1, -1, -1, -1 }, // 10
    { 10, 12, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 11, 13, 14, 15, 16, 17, -1, -1, -1, -1 },
    { 12, 14, 15, 16, 17, -1, -1, -1, -1, -1 },
    { 12, 13, 15, 16, 17, -1, -1, -1, -1, -1 },
    { 12, 13, 14, 16, 17, 18, -1, -1, -1, -1 }, // 15
    { 12, 13, 14, 15, 17, -1, -1, -1, -1, -1 },
    { 12, 13, 14, 15, 16, -1, -1, -1, -1, -1 },
    { 15, 19, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 18, 20, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 19, -1, -1, -1, -1, -1, -1, -1, -1, -1 }  // 20
    //{ 19,  0, -1, -1, -1, -1, -1, -1, -1, -1 }  // 20
};
struct {
    int x,  y;
    int dx, dy;
} G;

char ME = -1;
SDL_Window* WIN = NULL;
SDL_Renderer* REN = NULL;
SDL_Texture *IMG_left, *IMG_right, *IMG_up, *IMG_down, *IMG_space;

int main (int argc, char** argv) {
    assert(argc == 3);
    ME = atoi(argv[1]);
    int port = atoi(argv[2]);

#if 1
    if (ME == 5) {
        sleep(10);
    }
#endif

    p2p_loop (
        ME,
        port,
        FPS,
        sizeof(G), &G,
        cb_ini, cb_sim, cb_eff, cb_rec
    );
}

void cb_ini (int ini) {
    if (ini) {
        char title[255] = "[XX] P2P-TML: Peer-to-Peer Time Machine Library";
        title[1] = '0' + ME/10;
        title[2] = '0' + ME%10;
        WIN = SDL_CreateWindow (
            title,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SIZE, SIZE,
            SDL_WINDOW_SHOWN
        );
        assert(WIN != NULL);

        REN = SDL_CreateRenderer(WIN, -1, SDL_RENDERER_ACCELERATED);
        assert(REN != NULL);
        SDL_SetRenderDrawBlendMode(REN, SDL_BLENDMODE_BLEND);

        IMG_up    = IMG_LoadTexture(REN, "imgs/up.png");
        IMG_down  = IMG_LoadTexture(REN, "imgs/down.png");
        IMG_left  = IMG_LoadTexture(REN, "imgs/left.png");
        IMG_right = IMG_LoadTexture(REN, "imgs/right.png");
        IMG_space = IMG_LoadTexture(REN, "imgs/space.png");

        sleep(10);
        for (int i=0; i<10; i++) {
            int v = NET[(int)ME][i];
            if (v!=-1 && v>ME) {
                //printf(">>> %d <-> %d\n", ME, v);
                p2p_link("localhost", 5000+v, v);
            }
        }
    } else {
        SDL_DestroyTexture(IMG_up);
        SDL_DestroyTexture(IMG_down);
        SDL_DestroyTexture(IMG_left);
        SDL_DestroyTexture(IMG_right);
        SDL_DestroyTexture(IMG_space);
        SDL_DestroyRenderer(REN);
        SDL_DestroyWindow(WIN);
        SDL_Quit();
    }
}

void cb_sim (p2p_evt evt) {
    switch (evt.id) {
        case P2P_EVT_INIT:
            G.x  = 0;
            G.y  = 0;
            G.dx = 1;
            G.dy = 0;
            break;
        case P2P_EVT_TICK:
            G.x += G.dx * VEL;
            G.y += G.dy * VEL;
            break;
        case P2P_EVT_KEY:
            switch (evt.pay.i1) {
                case SDLK_UP:    { G.dx= 0; G.dy=-1; break; }
                case SDLK_DOWN:  { G.dx= 0; G.dy= 1; break; }
                case SDLK_LEFT:  { G.dx=-1; G.dy= 0; break; }
                case SDLK_RIGHT: { G.dx= 1; G.dy= 0; break; }
                case SDLK_SPACE: { G.dx= 0; G.dy= 0; break; }
            }
            break;
    }
}

void cb_eff (int trv) {
    SDL_SetRenderDrawColor(REN, 0xFF,0xFF,0xFF,0xFF);
    SDL_RenderClear(REN);
    SDL_Rect r = { G.x, G.y, DIM, DIM };
    SDL_SetRenderDrawColor(REN, 0xFF,0x00,0x00,0xFF);
    SDL_RenderFillRect(REN, &r);
    if (trv) {
        SDL_SetRenderDrawColor(REN, 0x77,0x77,0x77,0x77);
        SDL_RenderFillRect(REN, NULL);
#if 0
    } else {
        static int i = 0;
        if (++i%300 == 0) {
            flockfile(stdout);
            p2p_dump();
            printf(">>> %d,%d,%d,%d\n", G.x,G.y,G.dx,G.dy);
            funlockfile(stdout);
        }
#endif
    }
    SDL_RenderPresent(REN);
}

int cb_rec (SDL_Event* sdl, p2p_evt* evt) {
    if (sdl == NULL) return P2P_RET_NONE;
    switch (sdl->type) {
        case SDL_QUIT:
            return P2P_RET_QUIT;
        case SDL_KEYDOWN: {
            int key = sdl->key.keysym.sym;
            *evt = (p2p_evt) { P2P_EVT_KEY, 1, {.i1=key} };
            SDL_SetRenderDrawColor(REN, 0xFF,0x00,0x00,0xFF);
            switch (key) {
                case SDLK_UP:
                    SDL_RenderCopy(REN, IMG_up, NULL, NULL);
                    break;
                case SDLK_DOWN:
                    SDL_RenderCopy(REN, IMG_down, NULL, NULL);
                    break;
                case SDLK_LEFT:
                    SDL_RenderCopy(REN, IMG_left, NULL, NULL);
                    break;
                case SDLK_RIGHT:
                    SDL_RenderCopy(REN, IMG_right, NULL, NULL);
                    break;
                case SDLK_SPACE:
                    SDL_RenderCopy(REN, IMG_space, NULL, NULL);
                    break;
                case SDLK_x:
                    for (int i=0; i<10; i++) {
                        int v = NET[(int)ME][i];
                        if (v != -1) {
                            //printf(">>> %d <-> %d\n", ME, v);
                            p2p_unlink(v);
                        }
                    }
                    break;
                case SDLK_z:
                    for (int i=0; i<10; i++) {
                        int v = NET[(int)ME][i];
                        if (v != -1) {
                            //printf(">>> %d <-> %d\n", ME, v);
                            p2p_link("localhost", 5000+v, v);
                        }
                    }
                    break;
            }
            SDL_RenderPresent(REN);
	        SDL_Delay(50);
            return P2P_RET_REC;
        }
    }
    return P2P_RET_NONE;
}
