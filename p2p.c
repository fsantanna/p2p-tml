#include <assert.h>
#include <pthread.h>
#include <endian.h>
#include <SDL2/SDL_net.h>

#include "p2p.h"
#include "tcp.c"

#define LOCK()   pthread_mutex_lock(&G.lock)
#define UNLOCK() pthread_mutex_unlock(&G.lock);

typedef struct {
    TCPsocket s;
    uint32_t seq;
} p2p_net;

typedef struct {
    char     status;    // 0=receiving, -1=repeated, 1=ok
    uint8_t  src;
    uint32_t seq;
    uint32_t tick;
    p2p_evt  evt;
} p2p_pak;

static struct {
    uint8_t me;
    pthread_mutex_t lock;
    p2p_net net[P2P_MAX_NET];
    struct {
        int i;
        int n;
        p2p_pak buf[P2P_MAX_PAKS];
    } paks;
    struct {
        void (*sim) (p2p_evt);
        void (*eff) (int trv);
        int (*rec) (SDL_Event*,p2p_evt*);
    } cbs;
    struct {
        int      mpf;
        uint32_t nxt;
        int      tick;
    } time;
    struct {
        struct {
            int n;
            char* buf;
        } app;
        char* his[P2P_MAX_MEM];
    } mem;
} G = { -1, {}, {}, {0,0,{}}, {NULL,NULL,NULL}, {-1,-1,0}, {{-1,NULL},{}} };

void p2p_init (
    uint8_t me,
    int port,
    int fps,
    int mem_n,
    void* mem_buf,
    void (*cb_sim) (p2p_evt),
    void (*cb_eff) (int trv),
    int (*cb_rec) (SDL_Event*,p2p_evt*)
) {
    assert(me < P2P_MAX_NET);
    G.me = me;
    for (int i=0; i<P2P_MAX_NET; i++) {
        G.net[i] = (p2p_net) { NULL, 0 };
    }
    assert(pthread_mutex_init(&G.lock,NULL) == 0);
    assert(SDLNet_Init() == 0);
    IPaddress ip;
    assert(SDLNet_ResolveHost(&ip, NULL, port) == 0);
    TCPsocket s = SDLNet_TCP_Open(&ip);
    assert(s != NULL);
    G.net[G.me] = (p2p_net) { s, 0 };

    int mpf = 1000 / fps;
    assert(1000%fps == 0);

    G.cbs.sim = cb_sim;
    G.cbs.eff = cb_eff;
    G.cbs.rec = cb_rec;

    G.time.mpf = mpf;
    G.time.nxt = SDL_GetTicks()+mpf;

    G.cbs.sim((p2p_evt) { P2P_EVT_INIT, 0, {} });

    G.mem.app.n   = mem_n;
    G.mem.app.buf = mem_buf;
    for (int i=0; i<P2P_MAX_MEM; i++) {
        G.mem.his[i] = malloc(mem_n);
    }
    memcpy(G.mem.his[0], mem_buf, mem_n);
    //printf("<<< memcpy %d\n", 0);
}

void p2p_quit (void) {
    LOCK();
    for (int i=0; i<P2P_MAX_NET; i++) {
        SDLNet_TCP_Close(G.net[i].s);
    }
    UNLOCK();
    pthread_mutex_destroy(&G.lock);
    for (int i=0; i<P2P_MAX_MEM; i++) {
        free(G.mem.his[i]);
    }
	SDLNet_Quit();
}

static void p2p_bcast2 (p2p_pak* pak) {
    for (int i=0; i<P2P_MAX_NET; i++) {
        if (i == G.me) continue;
        TCPsocket s = G.net[i].s;
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
    tcp_send_u8(s, G.me);
    UNLOCK();
    uint8_t oth = tcp_recv_u8(s);

    LOCK();
    assert(oth < P2P_MAX_NET);
    assert(G.net[oth].s == NULL);
    G.net[oth] = (p2p_net) { s, 0 };
    UNLOCK();

    for (int i=0; i<G.paks.n; i++) {
        if (G.paks.buf[i].seq == 0) {
            break;
        }
        p2p_bcast2(&G.paks.buf[i]);
    }

    while (1) {
        LOCK();
        assert(G.paks.n < P2P_MAX_PAKS);
        p2p_pak* pak = &G.paks.buf[G.paks.n++];
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
        int cur = G.net[src].seq;
        if (seq > cur) {
            assert(seq == cur+1);
            G.net[src].seq++;
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

static int p2p_recv (p2p_evt* evt) {
    while (1) {
        TCPsocket s = SDLNet_TCP_Accept(G.net[G.me].s);
        if (s == NULL) {
            break;
        } else {
            IPaddress* ip = SDLNet_TCP_GetPeerAddress(s);
            assert(ip != NULL);
            pthread_t t;
            assert(pthread_create(&t, NULL,f,(void*)s) == 0);
        }
    }
    if (G.paks.i < G.paks.n) {
        switch (G.paks.buf[G.paks.i].status) {
            case 0:         // wait
                break;
            case -1:        // skip
                G.paks.i++;
                break;
            case 1:         // ready
                evt->id = G.paks.buf[G.paks.i].evt.id;
                evt->n  = G.paks.buf[G.paks.i].evt.n;
                evt->pay.i4._1 = be32toh(G.paks.buf[G.paks.i].evt.pay.i4._1);
                evt->pay.i4._2 = be32toh(G.paks.buf[G.paks.i].evt.pay.i4._2);
                evt->pay.i4._3 = be32toh(G.paks.buf[G.paks.i].evt.pay.i4._3);
                evt->pay.i4._4 = be32toh(G.paks.buf[G.paks.i].evt.pay.i4._4);
                G.paks.i++;
                return 1;
        }
    }
    return 0;
}

void p2p_bcast (uint32_t tick, p2p_evt* evt) {
    LOCK();
    uint32_t seq = ++G.net[G.me].seq;
    assert(G.paks.n < P2P_MAX_PAKS);
    p2p_pak* pak = &G.paks.buf[G.paks.n++];
    UNLOCK();
    *pak = (p2p_pak) { 1, G.me, seq, tick, {} };
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
        if (G.net[i].s == NULL) {
            printf("- ");
        } else {
            printf("%d ", G.net[i].seq);
        }
    }
    printf("\n");
}

void p2p_loop (void) {
    //printf("REC %d\n", G.time.tick);
    while (1) {
        p2p_evt evt;
        if (p2p_recv(&evt)) {
            G.cbs.sim(evt);
            G.cbs.eff(0);
        } else {
            uint32_t now = SDL_GetTicks();
            if (now < G.time.nxt) {
                SDL_WaitEventTimeout(NULL, G.time.nxt-now);
                now = SDL_GetTicks();
            }
            if (now >= G.time.nxt) {
                G.time.tick++;
                G.time.nxt += G.time.mpf;
                G.cbs.sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=G.time.tick} });
                if (G.time.tick % 100 == 0) {
                    assert(P2P_MAX_MEM > G.time.tick/100);
                    memcpy(G.mem.his[G.time.tick/100], G.mem.app.buf, G.mem.app.n);    // save w/o events
                    //printf("<<< memcpy %d\n", G.time.tick);
                }
                G.cbs.eff(0);
            } else {
                SDL_Event sdl;
                p2p_evt   evt;
                assert(SDL_PollEvent(&sdl));

                switch (G.cbs.rec(&sdl, &evt)) {
                    case P2P_RET_NONE:
                        break;
                    case P2P_RET_QUIT:
                        return;
                    case P2P_RET_REC: {
                        p2p_bcast(G.time.tick, &evt);
                        break;
                    }
                }
            }
        }
    }

#if 0
    G.cbs.eff(1);

    //printf("TRV %d\n", G.time.tick);
    uint32_t prv = SDL_GetTicks();
    uint32_t nxt = SDL_GetTicks();
    int tick = G.time.tick;
    int tot  = G.paks.n;
    int new  = -1;

    while (1) {
        uint32_t now = SDL_GetTicks();
        if (now < nxt) {
            SDL_WaitEventTimeout(NULL, nxt-now);
            now = SDL_GetTicks();
        }
        if (now >= nxt) {
            nxt += G.time.mpf;
        }

        if (new != -1) {
            assert(0<=new && new<=G.time.tick);
            memcpy(mem, G.mem.his[new/100], n);   // load w/o events
            int fst = new - new%100;
            //printf(">>> memcpy %d / fst %d\n", new/100, fst);

            // skip events before fst
            int e = 0;
            for (; e<G.paks.n && G.paks.buf[e].tick<fst; e++);

            for (int i=fst; i<=new; i++) {
                if (i > fst) { // first tick already loaded
                    G.cbs.sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=i} });
                }
                while (e<G.paks.n && G.paks.buf[e].tick==i) {
                    G.cbs.sim(G.paks.buf[e].evt);
                    e++;
                }
            }
            tick = new;
            tot  = e;
            //SDL_Delay(G.time.mpf);
            G.cbs.eff(1);
        }

        SDL_Event sdl;
        SDL_Event* ptr = SDL_PollEvent(&sdl) ? &sdl : NULL;

        switch (cb_trv(ptr, G.time.tick, tick, &new)) {
            case P2P_RET_NONE:
                new = -1;
                break;
            case P2P_RET_QUIT:
                return;
            case P2P_RET_REC:
                G.time.nxt += (SDL_GetTicks() - prv);
                //printf("OUT %d\n", tick);
                G.time.tick = tick;
                G.paks.i = G.paks.n = tot;
                //G.paks.i = MIN(G.paks.i, G.paks.n);
                goto _RET_REC_;
                break;
            case P2P_RET_TRV: {
                break;
            }
        }
    }
#endif
}
