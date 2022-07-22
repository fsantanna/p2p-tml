#if 0
#!/bin/sh
gcc -g -Wall `sdl2-config --cflags` p2p.c main.c -o xmain `sdl2-config --libs` -lpthread -lSDL2_net
exit
#endif

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <SDL.h>
#include "p2p.h"

#define FPS   50
#define WIN   400

/*
 *  0 - 1 - 2
 *   \     /
 *    - 3 -
 *      |
 *      4
 */
int NET[5][5] = {
    { 0, 1, 0, 1, 0 },
    { 1, 0, 1, 0, 0 },
    { 0, 1, 0, 1, 0 },
    { 1, 0, 1, 0, 1 },
    { 0, 0, 0, 1, 0 }
};

int main (int argc, char** argv) {
    assert(argc == 3);
    char me   = atoi(argv[1]);
    int  port = atoi(argv[2]);

    assert(SDL_Init(SDL_INIT_VIDEO) == 0);

    SDL_Window* win = SDL_CreateWindow (
        "P2P-TML: Peer-to-Peer Time Machine Library",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN, WIN,
        SDL_WINDOW_HIDDEN
    );
    assert(win != NULL);

    for (int i=0; i<5; i++) {
        for (int j=0; j<5; j++) {
            assert(NET[i][j] == NET[j][i]);
        }
    }

    void cb_sim (p2p_evt evt) {
        if (evt.id >= P2P_EVT_NEXT) {
            printf("<<< [%d] %d/%d\n", me, evt.id-P2P_EVT_NEXT, evt.pay.i1);
        }
    }
    void cb_eff (int trv) {
        if (trv) return;
        if (rand()%500 == 0) {
            printf("-=-=-=-=- dump from %d\n", me);
            p2p_dump();
            puts("-=-=-=-=-");
        }
        if (rand()%100 == 0) {
            static int i = 1;
            printf(">>>>>> [%d] %d/%d\n", me, me, i);
            p2p_evt evt = { P2P_EVT_NEXT+me, 1, {.i1=i++} };
            p2p_bcast(0, &evt);
        }
    }
    int cb_rec (SDL_Event* sdl, p2p_evt* p2p) {
        if (sdl->type == SDL_QUIT) {
            return P2P_RET_QUIT;
        }
        return P2P_RET_NONE;
    }

    srand(me);
    p2p_init(me,port,FPS,0,&me,cb_sim,cb_eff,cb_rec);

    sleep(1);
    for (int i=me+1; i<5; i++) {
        if (NET[(int)me][i]) {
            p2p_link("localhost", 5000+i, i);
        }
    }

    p2p_loop();
    p2p_quit();
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
