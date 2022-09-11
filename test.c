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

#define TST_TCK_MS   100
#define TST_TCK_TCKS (TST_TCK_MS*FPS/1000)

#define TST_SIM_MIN  3
#define TST_SIM_TOT  (TST_SIM_MIN*60*FPS)

#define TST_EVT_MIN  100  // 100 evt/min
#define TST_EVT_PEER (FPS*60*PEERS/TST_EVT_MIN)

#define PEERS 21
#define FPS   50
#define VEL    2

/*
 *                4 - 5                      13 - 14
 *               /     \                    /  \ /  \
 *  0 - 1 - 2 - 3       6 - 9 - 10 - 11 - 12 -- + -- 15 - 18 - 19 - 20
 *               \     /                    \  / \  /
 *                8 - 7                      17 - 16
 */
#if 1
int NET[PEERS][10] = {
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
#else
int NET[PEERS][10] = {
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // 0
    {  0,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};
#endif

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
        SDL_Delay(5000);
        for (int i=0; i<10; i++) {
            int v = NET[(int)ME][i];
            if (v!=-1 && v>ME) {
                //printf(">>> %d <-> %d\n", ME, v);
                p2p_link("localhost", 5000+v, v);
            }
        }
        SDL_Delay(rand()%10000);
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
    if (TICK%TST_TCK_TCKS == 0) {
        flockfile(stdout);
        printf("[%02d] TICK=%d pos=(%d,%d,%d,%d)\n", ME,TICK, G.x,G.y,G.dx,G.dy);
        fflush(stdout);
        funlockfile(stdout);
    }
}

int cb_rec (SDL_Event* sdl, p2p_evt* evt) {
    if (sdl != NULL) return P2P_RET_NONE;

    if (TICK > TST_SIM_TOT) {
        return P2P_RET_QUIT;
    } else if (TICK > TST_SIM_TOT-(30*FPS)) {
        return P2P_RET_NONE;
    }

    static int i = 0;
    static int _i = FPS;    // 1st after 1s
    if (i++ == _i) {
        _i += rand() % TST_EVT_PEER*2;
        if (i > 0) {
            int v = rand() % 5;
            flockfile(stdout);
            printf("[%02d] EVT=%d (%d)\n", ME,v, TICK);
            fflush(stdout);
            funlockfile(stdout);
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
