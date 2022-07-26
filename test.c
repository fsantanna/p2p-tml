#if 0
#!/bin/sh
gcc -g -Wall `sdl2-config --cflags` p2p.c test.c -o xtest `sdl2-config --libs` -lpthread -lSDL2_net -lSDL2_image
exit
#endif

#define TEST_OFFLINE

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

#define TST_TCK_MS   1000
#define TST_TCK_TCKS (TST_TCK_MS*FPS/1000)

#define TST_SIM_TOT  (P2P_TOTAL*FPS)
#define TST_EVT_PEER (FPS*60*PEERS/P2P_EVENTS)

#define PEERS 21
#define FPS   50
#define VEL    2

// 14 ou 7 saltos
/*
 *                4 - 6                      13 - 15
 *               /     \                    /  \ /  \
 *  0 - 1 - 2 - 3       8 - 9 - 10 - 11 - 12 -- + -- 17 - 18 - 19 - 20
 *               \     /                    \  / \  /
 *                5 - 7                      14 - 16
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
int  LAST = 0;

int main (int argc, char** argv) {
    assert(argc == 3);
    ME = atoi(argv[1]);
    int port = atoi(argv[2]);
    srand(port+time(NULL));

    {
        char s[255];
        sprintf(s, "files/file-%d.txt", ME);
        FILE* f = fopen(s, "rb");
        if (f != NULL) {
            fread(&LAST, sizeof(int), 1, f);
printf("READ = %d\n", LAST);
            fclose(f);
        }
    }

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
        //SDL_Delay(5000);
        for (int i=0; i<10; i++) {
            int v = NET[(int)ME][i];
            if (v != -1) {
                //printf(">>> %d <-> %d\n", ME, v);
                p2p_link("localhost", 5100+v, v);
            }
        }
#ifndef TEST_OFFLINE
        int XXX = rand() % 10000;
        SDL_Delay(XXX);
        printf("[%02d] rand %d\n", ME, XXX);
        fflush(stdout);
#endif
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
        printf("[%02d] tick %d pos=(%d,%d,%d,%d)\n", ME,TICK, G.x,G.y,G.dx,G.dy);
        fflush(stdout);
    }
}

static int first = 0;
static int FIRST = 0;

int cb_rec (SDL_Event* sdl, p2p_evt* evt) {
    static int evts = 0;
    if (first == 0) {
        first = 1;
        printf("[%02d] frst %d\n", ME, TICK);
        fflush(stdout);
    }

    if (sdl != NULL) return P2P_RET_NONE;

    if (TICK > TST_SIM_TOT) {
        printf("[%02d] evts %d\n", ME, evts);
        fflush(stdout);
        p2p_dump();
END = 1;
#ifdef TEST_OFFLINE
        p2p_travel(6000);
#else
        p2p_travel(15000);
#endif
        p2p_dump();
        printf("[%02d] TICK %d pos=(%d,%d,%d,%d)\n", ME,TICK, G.x,G.y,G.dx,G.dy);
        fflush(stdout);
        return P2P_RET_QUIT;
#ifdef TEST_OFFLINE
    } else if (TICK > TST_SIM_TOT-(30*FPS)) {   // -30s
#else
    } else if (TICK > TST_SIM_TOT-(10*FPS)) {   // -10s
#endif
        return P2P_RET_NONE;
    }

    static int i = 0;
    static int _i = FPS;    // 1st after 1s
    if (i++ == _i) {
        _i += rand() % TST_EVT_PEER*2;
        if (i > LAST) {
            {
                char s[255];
                sprintf(s, "files/file-%d.txt", ME);
                FILE* f = fopen(s, "wb");
                LAST = i;
printf("WRITE = %d\n", LAST);
                if (f != NULL) {
                    fwrite(&LAST, sizeof(int), 1, f);
                    fclose(f);
                }
            }
            if (FIRST == 0) {
                FIRST = 1;
                printf("[%02d] FRST %d %d\n", ME, TICK, SDL_GetTicks());
                fflush(stdout);
            }
            int v = rand() % 5;
            evts++;
            printf("[%02d] EVT=%d (%d)\n", ME,v, TICK);
            fflush(stdout);
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
