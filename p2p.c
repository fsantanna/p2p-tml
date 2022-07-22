#include <assert.h>
#include <pthread.h>
#include <endian.h>
#include <SDL2/SDL_net.h>

#include "p2p.h"
#include "tcp.c"

typedef struct {
    TCPsocket s;
    uint32_t seq;
} p2p_net;

static uint8_t ME = -1;

static p2p_net NET[P2P_MAX_NET];

struct {
    int i;
    int n;
    p2p_pak buf[P2P_MAX_PAKS];
} PAKS = { 0,0,{} };

static pthread_mutex_t L;
#define LOCK()   pthread_mutex_lock(&L)
#define UNLOCK() pthread_mutex_unlock(&L);

void p2p_init (uint8_t me, int port) {
    assert(me < P2P_MAX_NET);
    ME = me;
    for (int i=0; i<P2P_MAX_NET; i++) {
        NET[i] = (p2p_net) { NULL, 0 };
    }
    assert(pthread_mutex_init(&L,NULL) == 0);
    assert(SDLNet_Init() == 0);
    IPaddress ip;
    assert(SDLNet_ResolveHost(&ip, NULL, port) == 0);
    TCPsocket s = SDLNet_TCP_Open(&ip);
    assert(s != NULL);
    NET[ME] = (p2p_net) { s, 0 };
}

void p2p_quit (void) {
    LOCK();
    for (int i=0; i<P2P_MAX_NET; i++) {
        SDLNet_TCP_Close(NET[i].s);
    }
    UNLOCK();
    pthread_mutex_destroy(&L);
	SDLNet_Quit();
}

static void p2p_bcast2 (p2p_pak* pak) {
    for (int i=0; i<P2P_MAX_NET; i++) {
        if (i == ME) continue;
        TCPsocket s = NET[i].s;
        if (s == NULL) continue;
        LOCK();
        tcp_send_u8 (s, pak->src);
        tcp_send_u32(s, pak->seq);
        tcp_send_u32(s, pak->tick);
        tcp_send_u8 (s, pak->evt.id);
        tcp_send_u8 (s, pak->evt.n);
        tcp_send_n  (s, pak->evt.n*sizeof(uint32_t), (char*)&pak->evt.pay);
        UNLOCK();
    }
}

static void* f (void* arg) {
    TCPsocket s = (TCPsocket) arg;
    LOCK();
    tcp_send_u8(s, ME);
    UNLOCK();
    uint8_t oth = tcp_recv_u8(s);

    LOCK();
    assert(oth < P2P_MAX_NET);
    assert(NET[oth].s == NULL);
    NET[oth] = (p2p_net) { s, 0 };
    UNLOCK();

    for (int i=0; i<PAKS.n; i++) {
        if (PAKS.buf[i].seq == 0) {
            break;
        }
        p2p_bcast2(&PAKS.buf[i]);
    }

    while (1) {
        LOCK();
        assert(PAKS.n < P2P_MAX_PAKS);
        p2p_pak* pak = &PAKS.buf[PAKS.n++];
        pak->status = 0;   // not ready
        UNLOCK();

        uint8_t  src  = tcp_recv_u8(s);
        uint32_t seq  = tcp_recv_u32(s);
        uint32_t tick = tcp_recv_u32(s);
        uint8_t  id   = tcp_recv_u8(s);
        uint8_t  n    = tcp_recv_u8(s);
        *pak = (p2p_pak) { 0, src, seq, tick, {id,n,{}} };
        tcp_recv_n(s, n*sizeof(uint32_t), (char*)&pak->evt.pay);

        LOCK();
        pak->status = -1;
        int cur = NET[src].seq;
        if (seq > cur) {
            assert(seq == cur+1);
            NET[src].seq++;
            pak->status = 1;
        }
        UNLOCK();

        if (seq > cur) {
            p2p_bcast2(pak);
        }
    }

    SDLNet_TCP_Close(s);
    return NULL;
}

int p2p_recv (p2p_evt* evt) {
    while (1) {
        TCPsocket s = SDLNet_TCP_Accept(NET[ME].s);
        if (s == NULL) {
            break;
        } else {
            IPaddress* ip = SDLNet_TCP_GetPeerAddress(s);
            assert(ip != NULL);
            pthread_t t;
            assert(pthread_create(&t, NULL,f,(void*)s) == 0);
        }
    }
    if (PAKS.i < PAKS.n) {
        switch (PAKS.buf[PAKS.i].status) {
            case 0:         // wait
                break;
            case -1:        // skip
                PAKS.i++;
                break;
            case 1:         // ready
                evt->id = PAKS.buf[PAKS.i].evt.id;
                evt->n  = PAKS.buf[PAKS.i].evt.n;
                evt->pay.i4._1 = be32toh(PAKS.buf[PAKS.i].evt.pay.i4._1);
                evt->pay.i4._2 = be32toh(PAKS.buf[PAKS.i].evt.pay.i4._2);
                evt->pay.i4._3 = be32toh(PAKS.buf[PAKS.i].evt.pay.i4._3);
                evt->pay.i4._4 = be32toh(PAKS.buf[PAKS.i].evt.pay.i4._4);
                PAKS.i++;
                return 1;
        }
    }
    return 0;
}

void p2p_bcast (uint32_t tick, p2p_evt* evt) {
    LOCK();
    uint32_t seq = ++NET[ME].seq;
    assert(PAKS.n < P2P_MAX_PAKS);
    p2p_pak* pak = &PAKS.buf[PAKS.n++];
    UNLOCK();
    *pak = (p2p_pak) { 1, ME, seq, tick, {} };
    pak->evt.id        = evt->id;
    pak->evt.n         = evt->n;
    pak->evt.pay.i4._1 = htobe32(evt->pay.i4._1);
    pak->evt.pay.i4._2 = htobe32(evt->pay.i4._2);
    pak->evt.pay.i4._3 = htobe32(evt->pay.i4._3);
    pak->evt.pay.i4._4 = htobe32(evt->pay.i4._4);
    p2p_bcast2(pak);
}

void p2p_link (char* host, int port, uint8_t oth) {
	IPaddress ip;
	assert(SDLNet_ResolveHost(&ip, host, port) == 0);
	TCPsocket s = SDLNet_TCP_Open(&ip);
    assert(s != NULL);
    pthread_t t;
    assert(pthread_create(&t, NULL,f,(void*)s) == 0);
}

void p2p_dump (void) {
    for (int i=0; i<5; i++) {
        if (NET[i].s == NULL) {
            printf("- ");
        } else {
            printf("%d ", NET[i].seq);
        }
    }
    printf("\n");
}

void p2p_loop (int fps, int n, void* mem, void(*cb_sim)(p2p_evt), void(*cb_eff)(int), int(*cb_rec)(SDL_Event*,p2p_evt*)) {
    char MEM[P2P_MAX_MEM][n];
    int mpf = 1000 / fps;
    assert(1000%fps == 0);

    struct {
        uint32_t nxt;
        int      mpf;
        int      tick;
    } S = { SDL_GetTicks()+mpf, mpf, 0 };

    cb_sim((p2p_evt) { P2P_EVT_INIT, 0, {} });
    memcpy(MEM[0], mem, n);
    //printf("<<< memcpy %d\n", 0);

_RET_REC_: {

    //printf("REC %d\n", S.tick);
    while (1) {
        p2p_evt evt;
        if (p2p_recv(&evt)) {
            cb_sim(evt);
            cb_eff(0);
        } else {
            uint32_t now = SDL_GetTicks();
            if (now < S.nxt) {
                SDL_WaitEventTimeout(NULL, S.nxt-now);
                now = SDL_GetTicks();
            }
            if (now >= S.nxt) {
                S.tick++;
                S.nxt += S.mpf;
                cb_sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=S.tick} });
                if (S.tick % 100 == 0) {
                    assert(P2P_MAX_MEM > S.tick/100);
                    memcpy(MEM[S.tick/100], mem, n);    // save w/o events
                    //printf("<<< memcpy %d\n", S.tick);
                }
                cb_eff(0);
            } else {
                SDL_Event sdl;
                p2p_evt   evt;
                assert(SDL_PollEvent(&sdl));

                switch (cb_rec(&sdl, &evt)) {
                    case P2P_RET_NONE:
                        break;
                    case P2P_RET_QUIT:
                        return;
                    case P2P_RET_REC: {
                        p2p_bcast(S.tick, &evt);
                        break;
                    }
                }
            }
        }
    }
}

_RET_TRV_: {

    cb_eff(1);

    //printf("TRV %d\n", S.tick);
    uint32_t prv = SDL_GetTicks();
    uint32_t nxt = SDL_GetTicks();
    int tick = S.tick;
    int tot  = PAKS.n;
    int new  = -1;

    while (1) {
        uint32_t now = SDL_GetTicks();
        if (now < nxt) {
            SDL_WaitEventTimeout(NULL, nxt-now);
            now = SDL_GetTicks();
        }
        if (now >= nxt) {
            nxt += S.mpf;
        }

        if (new != -1) {
            assert(0<=new && new<=S.tick);
            memcpy(mem, MEM[new/100], n);   // load w/o events
            int fst = new - new%100;
            //printf(">>> memcpy %d / fst %d\n", new/100, fst);

            // skip events before fst
            int e = 0;
            for (; e<PAKS.n && PAKS.buf[e].tick<fst; e++);

            for (int i=fst; i<=new; i++) {
                if (i > fst) { // first tick already loaded
                    cb_sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=i} });
                }
                while (e<PAKS.n && PAKS.buf[e].tick==i) {
                    cb_sim(PAKS.buf[e].evt);
                    e++;
                }
            }
            tick = new;
            tot  = e;
            //SDL_Delay(S.mpf);
            cb_eff(1);
        }

#if 0
        SDL_Event sdl;
        SDL_Event* ptr = SDL_PollEvent(&sdl) ? &sdl : NULL;

        switch (cb_trv(ptr, S.tick, tick, &new)) {
            case P2P_RET_NONE:
                new = -1;
                break;
            case P2P_RET_QUIT:
                return;
            case P2P_RET_REC:
                S.nxt += (SDL_GetTicks() - prv);
                //printf("OUT %d\n", tick);
                S.tick = tick;
                PAKS.i = PAKS.n = tot;
                //PAKS.i = MIN(PAKS.i, PAKS.n);
                goto _RET_REC_;
                break;
            case P2P_RET_TRV: {
                break;
            }
        }
#endif
    }
}
}
