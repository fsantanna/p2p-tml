#if 0
#!/bin/sh
gcc -g -Wall `sdl2-config --cflags` p2p.c test.c -o xtest `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image
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

/*
 *                   11                    31
 *                  /  \                  /  \
 *  0 - ... - 9 - 10    15 - 20 - ... - 30 -- 35 - 40 - ... 49
 *                  \  /                  \  /
 *                   19                    39
 */
int NET[50][10] = {
    {  1,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, // 0
    {  0,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4,  6, -1, -1, -1, -1, -1, -1, -1, -1 }, // 5
    {  5,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  6,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  7,  9, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  8, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  9, 11, 19, -1, -1, -1, -1, -1, -1, -1 }, // 10
    { 10, 12, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 11, 13, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 12, 14, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 13, 15, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 14, 16, 20, -1, -1, -1, -1, -1, -1, -1 }, // 15
    { 15, 17, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 16, 18, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 17, 19, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 18, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 15, 21, -1, -1, -1, -1, -1, -1, -1, -1 }, // 20
    { 20, 22, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 21, 23, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 22, 24, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 23, 25, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 24, 26, -1, -1, -1, -1, -1, -1, -1, -1 }, // 25
    { 25, 27, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 26, 28, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 27, 29, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 28, 30, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 29, 31, 32, 33, 34, 35, 36, 37, 38, 39 }, // 30
    { 30, 32, 33, 34, 35, 36, 37, 38, 39, -1 },
    { 30, 31, 33, 34, 35, 36, 37, 38, 39, -1 },
    { 30, 31, 32, 34, 35, 36, 37, 38, 39, -1 },
    { 30, 31, 32, 33, 35, 36, 37, 38, 39, -1 },
    { 30, 31, 32, 33, 34, 36, 37, 38, 39, 40 }, // 35
    { 30, 31, 32, 33, 34, 35, 37, 38, 39, -1 },
    { 30, 31, 32, 33, 34, 35, 36, 38, 39, -1 },
    { 30, 31, 32, 33, 34, 35, 36, 37, 39, -1 },
    { 30, 31, 32, 33, 34, 35, 36, 37, 38, -1 },
    { 35, 41, -1, -1, -1, -1, -1, -1, -1, -1 }, // 40
    { 40, 42, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 41, 43, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 42, 44, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 43, 45, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 44, 46, -1, -1, -1, -1, -1, -1, -1, -1 }, // 45
    { 45, 47, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 46, 48, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 47, 49, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 48, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
};

struct {
    int x,  y;
    int dx, dy;
} G;

int  TICK = 0;
char ME = -1;
SDL_Window* WIN = NULL;
SDL_Renderer* REN = NULL;
SDL_Texture *IMG_left, *IMG_right, *IMG_up, *IMG_down, *IMG_space;

int main (int argc, char** argv) {
    assert(argc == 3);
    ME = atoi(argv[1]);
    int port = atoi(argv[2]);

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
        WIN = SDL_CreateWindow (
            "P2P-TML: Peer-to-Peer Time Machine Library",
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

        sleep(1);
        for (int i=0; i<5; i++) {
            int v = NET[(int)ME][i];
            if (v>=0 && v>ME) {
                p2p_link("localhost", 5000+i, i);
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
            TICK = MAX(TICK, evt.pay.i1);
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
    if (trv) return;
    static int i = 0;
    if (++i%300 == 0) {
        flockfile(stdout);
        printf(">>> [%d:%d] (%d,%d,%d,%d)\n", ME,TICK, G.x,G.y,G.dx,G.dy);
        funlockfile(stdout);
    }
}

int cb_rec (SDL_Event* sdl, p2p_evt* evt) {
    if (sdl != NULL) return P2P_RET_NONE;

    if (TICK > 1000) {
        return P2P_RET_QUIT;
    }

    static int i = 0;
    static int _i = 0;
    if (++i == _i) {
        _i += rand() % 300;
        if (i > 0) {
            flockfile(stdout);
            printf(">>> [%d] EVT\n", ME);
            funlockfile(stdout);
        }
    }
 return P2P_RET_NONE;
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
            }
            SDL_RenderPresent(REN);
	        SDL_Delay(50);
            return P2P_RET_REC;
        }
    }
    return P2P_RET_NONE;
}
