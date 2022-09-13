#include <assert.h>
#include <pthread.h>
#include <endian.h>
#include <SDL2/SDL_net.h>

#include "p2p.h"
#include "tcp.c"

#define LOCK()    pthread_mutex_lock(&G.lock)
#define UNLOCK()  pthread_mutex_unlock(&G.lock);
#define PAK(i)    G.paks.buf[i]
//#define DELTA_TICKS (MAX(2, P2P_DELTA/G.time.mpf))
#define DELTA_TICKS (P2P_DELTA/G.time.mpf)

typedef struct {
    TCPsocket s;
    uint32_t seq;
} p2p_net;

typedef struct {
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

struct {
    int start;
    int tcks;
    int fwds_i;
    int fwds_s;
    int baks_i;
    int baks_s;
    int travel;
} T = { 0, 0, 0, 0, 0, 0, 0 };

static void p2p_bcast2 (p2p_pak pak) {
    for (int i=0; i<P2P_MAX_NET; i++) {
        if (i == G.me) continue;
        TCPsocket s = G.net[i].s;
        if (s == NULL) continue;
        LOCK();
        tcp_send_u8 (s, pak.src);
        tcp_send_u32(s, pak.seq);
        tcp_send_u32(s, pak.tick);
        tcp_send_u8 (s, pak.evt.id);
        tcp_send_u8 (s, pak.evt.n);
        tcp_send_u32(s, pak.evt.pay.i4._1);
        tcp_send_u32(s, pak.evt.pay.i4._2);
        tcp_send_u32(s, pak.evt.pay.i4._3);
        tcp_send_u32(s, pak.evt.pay.i4._4);
        UNLOCK();
    }
}

static void p2p_reorder (void) {
    // reorder to respect tick/src
    assert(G.paks.n > 0);
    int i = G.paks.n - 1;
    while (1) {
        if (i == 0) {
            break;      // nothing below to compare
        }
        p2p_pak* hi = &PAK(i);
        p2p_pak* lo = &PAK(i-1);
        if (hi->tick > lo->tick) {
            break;      // higher tick
        } else if (hi->tick==lo->tick && hi->src>=lo->src) {
            break;      // same tick, but higher src
        }

        // reorder: lower tick || same tick && lower src
        p2p_pak tmp = *hi;
        PAK(i) = *lo;
        PAK(i-1) = tmp;
        if (i == G.paks.i) {
            G.paks.i--;
        }
        i--;
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

    int N = G.paks.n;
    UNLOCK();

    for (int i=0; i<N; i++) {
        p2p_bcast2(PAK(i));
    }

    while (1) {
        uint8_t  src  = tcp_recv_u8(s);
        uint32_t seq  = tcp_recv_u32(s);
        uint32_t tick = tcp_recv_u32(s);
        uint8_t  id   = tcp_recv_u8(s);
        uint8_t  n    = tcp_recv_u8(s);
        uint32_t i1   = tcp_recv_u32(s);
        uint32_t i2   = tcp_recv_u32(s);
        uint32_t i3   = tcp_recv_u32(s);
        uint32_t i4   = tcp_recv_u32(s);
        //SDL_Delay(P2P_LATENCY); // - P2P_LATENCY/10 + rand()%P2P_LATENCY/5);
        SDL_Delay(P2P_LATENCY - P2P_LATENCY/10 + rand()%P2P_LATENCY/5);
//puts("-=-=-=-=-");
        p2p_pak pak = { src, seq, tick, {id,n,{.i4={i1,i2,i3,i4}}} };

        LOCK();
        assert(G.paks.n < P2P_MAX_PAKS);

        int cur = G.net[src].seq;
        if (seq > cur) {
#if 1 // remove when assert no longer fails
            if (seq != cur+1) {
                printf(">>> [%02d] %d == %d+1\n", G.me, seq, cur);
            }
#endif
            assert(seq == cur+1);
            G.net[src].seq++;
            PAK(G.paks.n++) = pak;
            p2p_reorder();
        }
        UNLOCK();

        if (seq > cur) {
            p2p_bcast2(pak);
        }
    }

    SDLNet_TCP_Close(s);
    return NULL;
}

void p2p_bcast (uint32_t tick, p2p_evt* evt) {
    LOCK();
    uint32_t seq = ++G.net[G.me].seq;
    assert(G.paks.n < P2P_MAX_PAKS);
    p2p_pak pak = { G.me, seq, tick, *evt };
    PAK(G.paks.n++) = pak;
    p2p_reorder();
    UNLOCK();
    p2p_bcast2(pak);
}

void p2p_link (char* host, int port, uint8_t oth) {
    IPaddress ip;
    assert(SDLNet_ResolveHost(&ip, host, port) == 0);
    TCPsocket s = SDLNet_TCP_Open(&ip);
#if 0
if (s == NULL) {
    printf("%d %d\n", port, oth);
}
#endif
    assert(s != NULL);
    pthread_t t;
    assert(pthread_create(&t, NULL,f,(void*)s) == 0);
}

void p2p_dump (void) {
    printf("[%d] ", G.me);
    for (int i=0; i<5; i++) {
        printf("%d ", G.net[i].seq);
    }
    puts("");
    printf("[%d] ", G.me);
    int sum = 0;
    for (int i=0; i<G.paks.n; i++) {
        int pay = PAK(i).evt.pay.i1;
        pay = pay-(pay>999?SDLK_RIGHT:0);
        sum += pay + PAK(i).tick;
        printf("%d(%d) ", pay, PAK(i).tick);
    }
    printf("= %d\n", sum);
}

static void p2p_travel (int to) {
    assert(0<=to && to<=G.time.tick);
    memcpy(G.mem.app.buf, G.mem.his[to/P2P_HIS_TICKS], G.mem.app.n);   // load w/o events

    int fst = to - to%P2P_HIS_TICKS;
    int e = 0;

    // skip events before fst
    for (; e<G.paks.n && PAK(e).tick<fst; e++);

    for (int i=fst; i<=to; i++) {
        if (i > fst) { // first tick already loaded
            G.cbs.sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=i} });
        }
        while (e<G.paks.n && PAK(e).tick==i) {
            G.cbs.sim(PAK(e).evt);
            e++;
        }
    }
}

void p2p_loop_net (void);
int  p2p_loop_sdl (void);

void p2p_loop (
    uint8_t me,
    int port,
    int fps,
    int mem_n,
    void* mem_buf,
    void (*cb_ini) (int),
    void (*cb_sim) (p2p_evt),
    void (*cb_eff) (int trv),
    int (*cb_rec) (SDL_Event*,p2p_evt*)
) {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);

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
    T.start = fps*60*1;

    G.cbs.sim((p2p_evt) { P2P_EVT_INIT, 0, {} });

    G.mem.app.n   = mem_n;
    G.mem.app.buf = mem_buf;
    for (int i=0; i<P2P_MAX_MEM; i++) {
        G.mem.his[i] = malloc(mem_n);
    }
    memcpy(G.mem.his[0], mem_buf, mem_n);

    cb_ini(1);
    G.time.nxt = SDL_GetTicks()+mpf;

    while (1) {
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
        p2p_loop_net();
        if (p2p_loop_sdl()) {
            break;
        }
    }

    cb_ini(0);
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
    printf("[%02d] tcks %d\n", G.me, T.tcks);
    printf("[%02d] fwds %d %d\n", G.me, T.fwds_s, T.fwds_i);
    printf("[%02d] baks %d %d\n", G.me, T.baks_s, T.baks_i);
}

void p2p_loop_net (void) {
    LOCK();
T.travel = 0;
    while (1) {
        int next = PAK(G.paks.i).tick;
        int last = PAK(G.paks.n-1).tick;

        static int islate = 0;
        int oldislate = islate;
        islate = 0;

        if (G.paks.i == G.paks.n) {
            break;
        } else if (next == G.time.tick) {
            // i'm just in time, handle next event in real time
            G.cbs.sim(PAK(G.paks.i).evt);
            G.cbs.eff(0);
            G.paks.i++;
        } else if (last > G.time.tick+DELTA_TICKS) {
            // i'm too much in the past, need to hurry
            int dif = last - G.time.tick - DELTA_TICKS;
            assert(dif > 0);
T.travel = 1;
            int x = G.time.mpf*1000/MAX(2000,2*dif*G.time.mpf);
            G.time.nxt -= G.time.mpf;
            G.time.nxt += x;
#if 1 // paper instrumentation
            if (G.time.tick > T.start) {
                if (!oldislate) {
//printf("[%02d] late=%d\n", G.me, G.time.tick);
//printf("[%02d] etal=%d\n", G.me, G.time.tick);
                    T.fwds_i++;
                    islate = 1;
                }
                T.fwds_s++;
            }
            //printf("[%02d] GOFWD=%d [%d = 2*%d*%d/1000]\n", G.me, dif, x, dif, G.time.mpf);
#endif
            break;
        } else if (next < G.time.tick) {
            // i'm in the future, need to go back and forth
            int dt = MIN(G.time.mpf/2, 500/(G.time.tick-next)/2);
            G.paks.n--;     // do not include deviating event
#if 1 // paper instrumentation
            if (G.time.tick > T.start) {
                T.baks_i += 1;
                T.baks_s += (G.time.tick-next);
            }
            //printf("[%02d] GOBAK=%d\n", G.me, G.time.tick-next);
#endif
            for (int j=G.time.tick-1; j>next; j--) {
                p2p_travel(j);
                G.cbs.eff(1);
                //SDL_Delay(dt);
            }
            G.paks.n++;     // now include it and move forward
            for (int j=next; j<=G.time.tick; j++) {
                p2p_travel(j);
                G.cbs.eff(1);
                //SDL_Delay(dt);
            }
            G.time.nxt = SDL_GetTicks() + G.time.mpf;
            G.paks.i++;
        } else {
            break;
        }
    }
    UNLOCK();
}

int p2p_loop_sdl (void) {
    uint32_t now = SDL_GetTicks();
    //assert(now <= G.time.nxt);
    if (now < G.time.nxt) {
        SDL_WaitEventTimeout(NULL, G.time.nxt-now);
        now = SDL_GetTicks();
    }

    // TICK
    if (now >= G.time.nxt) {
        G.time.tick++;
        G.time.nxt += G.time.mpf;
        G.cbs.sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=G.time.tick} });
        if (G.time.tick % P2P_HIS_TICKS == 0) {
            assert(P2P_MAX_MEM > G.time.tick/P2P_HIS_TICKS);
            memcpy(G.mem.his[G.time.tick/P2P_HIS_TICKS], G.mem.app.buf, G.mem.app.n);    // save w/o events
        }
        if (G.time.tick > T.start) {
            T.tcks++;
        }
        G.cbs.eff(0);
    }

    // EVENT
    {
        SDL_Event sdl;
        p2p_evt   evt;
        int has = SDL_PollEvent(&sdl);

#if 1
        if (has && sdl.type==SDL_KEYDOWN && sdl.key.keysym.sym==SDLK_ESCAPE) {
            LOCK();
            p2p_travel(G.time.tick);
            G.cbs.eff(0);
            p2p_dump();
            UNLOCK();
        }
#endif

if (!T.travel) {
        switch (G.cbs.rec(has?&sdl:NULL, &evt)) {
            case P2P_RET_NONE:
                break;
            case P2P_RET_QUIT:
                return 1;
            case P2P_RET_REC: {
                //p2p_bcast(G.time.tick-2+rand()%5, &evt);
                int t1 = G.time.tick + DELTA_TICKS;
                int t2 = (G.paks.n == 0) ? 0 : PAK(G.paks.n-1).tick+1;
                p2p_bcast(MAX(t1,t2), &evt);
                break;
            }
        }
}
    }
    return 0;
}
