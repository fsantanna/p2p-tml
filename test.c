#if 0
#!/bin/sh
gcc -g -Wall `sdl2-config --cflags` p2p.c test.c -o xtest `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image
exit
#endif

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "p2p.h"

enum {
    P2P_EVT_KEY = P2P_EVT_NEXT
};

void cb_ini (int);
void cb_sim (p2p_evt);
void cb_eff (int trv);
int  cb_rec (SDL_Event* sdl, p2p_evt* evt);

#define FPS 50
#define VEL  2

/*
 *                   11                    31
 *                  /  \                  /  \
 *  0 - ... - 9 - 10    15 - 20 - ... - 30 -- 35 - 40 - ... 49
 *                  \  /                  \  /
 *                   19                    39
 */
int NET[50][10] = {
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // 0
    {  0,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1,  -9, -1, -1, -1, -1, -1, -1, -1, -1 },
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

int main (int argc, char** argv) {
    assert(argc == 3);
    ME = atoi(argv[1]);
    int port = atoi(argv[2]);
    srand(port+time(NULL));

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
        sleep(1);
        for (int i=0; i<5; i++) {
            int v = NET[(int)ME][i];
            if (v!=-1 && v>ME) {
                printf(">>> %d <-> %d\n", ME, v);
                p2p_link("localhost", 5000+v, v);
            }
        }
    } else {
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
    if (TICK%300 == 0) {
        flockfile(stdout);
        printf(">>> [%d:%d] (%d,%d,%d,%d)\n", ME,TICK, G.x,G.y,G.dx,G.dy);
        funlockfile(stdout);
    }
}

int cb_rec (SDL_Event* sdl, p2p_evt* evt) {
    if (sdl != NULL) return P2P_RET_NONE;

    if (TICK > 10000) {
        return P2P_RET_QUIT;
    }

    static int i = 0;
    static int _i = 0;
    if (i++ == _i) {
        _i += rand() % 300;
        if (i > 0) {
            int v = rand() % 5;
            flockfile(stdout);
            funlockfile(stdout);
            printf(">>> [%d] EVT=%d\n", ME,v);
            int key;
            switch (v) {
                case 0: { key=SDLK_LEFT;  break; }
                case 1: { key=SDLK_RIGHT; break; }
                case 2: { key=SDLK_UP;    break; }
                case 3: { key=SDLK_DOWN;  break; }
                case 4: { key=SDLK_SPACE; break; }
            }
            *evt = (p2p_evt) { P2P_EVT_KEY, 1, {.i1=key} };
            return P2P_RET_REC;
        }
    }
    return P2P_RET_NONE;
}
