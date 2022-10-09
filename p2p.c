#include <assert.h>
#include <pthread.h>
#include <endian.h>
#include <SDL2/SDL_net.h>

#include "p2p.h"
#include "tcp.c"

#define LOCK()    pthread_mutex_lock(&G.lock)
#define UNLOCK()  pthread_mutex_unlock(&G.lock);
#define PAK(i)    G.paks.buf[i]
//#define DELTA_TICKS (MAX(2, P2P_LATENCY_N/G.time.mpf))
#define DELTA_TICKS (P2P_LATENCY_N/G.time.mpf)

typedef struct {
    TCPsocket s;
    uint32_t  tick;
} p2p_net;

typedef struct {
    uint8_t  src;
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
        int      dif2;
        int      mpf2;
        int      max;
    } time;
    struct {
        struct {
            int n;
            char* buf;
        } app;
        char* his[P2P_MAX_MEM];
    } mem;
} G = { -1, {}, {}, {0,0,{}}, {NULL,NULL,NULL}, {-1,-1,0,0,0,0}, {{-1,NULL},{}} };

struct {
    int wait;
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
        LOCK();
        TCPsocket s = G.net[i].s;
        if (s == NULL) {
            // skip
        } else {
            int ok = 1;
            ok = ok && tcp_send_u8 (s, pak.src);
            ok = ok && tcp_send_u32(s, pak.tick);
            ok = ok && tcp_send_u8 (s, pak.evt.id);
            ok = ok && tcp_send_u8 (s, pak.evt.n);
            ok = ok && tcp_send_u32(s, pak.evt.pay.i4._1);
            ok = ok && tcp_send_u32(s, pak.evt.pay.i4._2);
            ok = ok && tcp_send_u32(s, pak.evt.pay.i4._3);
            ok = ok && tcp_send_u32(s, pak.evt.pay.i4._4);
            if (!ok) {
                G.net[i].s = NULL;
            }
        }
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
    int ok = tcp_send_u8(s, G.me);
    UNLOCK();

    uint8_t oth;
    if (!ok || !tcp_recv_u8(s, &oth)) {
        goto _OUT2_;
    }

    LOCK();
    assert(oth < P2P_MAX_NET);
#if 1
    G.net[oth].s = s;
#else
    if (G.net[oth].s == NULL) {
        G.net[oth].s = s;
    } else {
        UNLOCK();
        goto _OUT2_;
    }
#endif
    int N = G.paks.n;
    UNLOCK();

    for (int i=0; i<N; i++) {
        p2p_bcast2(PAK(i));
    }

    while (1) {
        uint8_t  src;
        uint32_t tick;
        uint8_t  id, n;
        uint32_t i1, i2, i3, i4;

        int ok = 1;
        ok = ok && tcp_recv_u8 (s, &src);
        ok = ok && tcp_recv_u32(s, &tick);
        ok = ok && tcp_recv_u8 (s, &id);
        ok = ok && tcp_recv_u8 (s, &n);
        ok = ok && tcp_recv_u32(s, &i1);
        ok = ok && tcp_recv_u32(s, &i2);
        ok = ok && tcp_recv_u32(s, &i3);
        ok = ok && tcp_recv_u32(s, &i4);
        if (!ok) {
            goto _OUT1_;
        }

#if 1
        if (id == P2P_EVT_SYNC) {
            LOCK();
            G.time.max = MAX(G.time.max, tick);
            UNLOCK();
            continue;
        }
#endif

//puts("-=-=-=-=-");
        p2p_pak pak = { src, tick, {id,n,{.i4={i1,i2,i3,i4}}} };

        LOCK();
        assert(G.paks.n < P2P_MAX_PAKS);

        int cur = G.net[src].tick;
        if (tick > cur) {
//printf("[%02d] RCV %d from %d\n", G.me, tick, oth);
//fflush(stdout);
            G.net[src].tick = tick;
            PAK(G.paks.n++) = pak;
            p2p_reorder();
        }
        UNLOCK();

        if (tick > cur) {
            p2p_bcast2(pak);
        }
    }

_OUT1_:
    LOCK();
    G.net[oth].s = NULL;
    UNLOCK();
_OUT2_:
    SDLNet_TCP_Close(s);
    return NULL;
}

void p2p_bcast (uint32_t tick, p2p_evt* evt) {
    LOCK();
    p2p_pak pak = { G.me, tick, *evt };
    G.net[G.me].tick = tick;
    assert(G.paks.n < P2P_MAX_PAKS);
    PAK(G.paks.n++) = pak;
    p2p_reorder();
    UNLOCK();
    p2p_bcast2(pak);
}

void p2p_link (char* host, int port, uint8_t oth) {
    LOCK();
    if (G.net[oth].s != NULL) {
        // already connected
    } else {
        IPaddress ip;
        assert(SDLNet_ResolveHost(&ip, host, port) == 0);
        TCPsocket s = SDLNet_TCP_Open(&ip);
#if 0
if (s == NULL) {
    printf("%d %d %s\n", port, oth, SDLNet_GetError());
    fflush(stdout);
}
//assert(s != NULL);
#endif
        if (s != NULL) {
            pthread_t t;
            assert(pthread_create(&t, NULL,f,(void*)s) == 0);
        }
    }
    UNLOCK();
}

#if 0
void p2p_unlink (uint8_t oth) {
    LOCK();
    //assert(G.net[oth].s != NULL);
    //SDLNet_TCP_Close(G.net[oth].s);
    G.net[oth].s = NULL;
    UNLOCK();
}
#endif

void p2p_dump (void) {
    printf("[%02d] AAA ", G.me);
    int sum1 = 0;
    for (int i=0; i<21; i++) {
        sum1 += i * G.net[i].tick;
        printf("%d ", G.net[i].tick);
    }
    puts("");
    printf("[%02d] BBB ", G.me);
    int sum2 = 0;
    for (int i=0; i<G.paks.n; i++) {
        int pay = PAK(i).evt.pay.i1;
        pay = pay-(pay>999?SDLK_RIGHT:0);
        sum2 += i*pay + i*PAK(i).tick;
        printf("%d/%d ", PAK(i).src, PAK(i).tick);
    }
    printf("= %d\n", sum2);
    printf("[%02d] XXX = %d\n", G.me, sum1);
    printf("[%02d] YYY = %d\n", G.me, sum2);
    fflush(stdout);
}

int END = 1;
void p2p_travel (int to) {
#if 0
    if (!(0<=to && to<=G.time.tick)) {
        printf("[%02d] ERR to=%d / tk=%d\n", G.me, to, G.time.tick);
        fflush(stdout);
    }
#endif
    assert(0<=to && to<=G.time.tick);
    memcpy(G.mem.app.buf, G.mem.his[END?0:to/P2P_HIS_TICKS], G.mem.app.n);   // load w/o events

    int fst = END ? 0 : to - to%P2P_HIS_TICKS;
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
#if 1
if (s == NULL) {
    printf("%d %d %s\n", G.me, port, SDLNet_GetError());
    fflush(stdout);
}
#endif
    assert(s != NULL);
    G.net[G.me] = (p2p_net) { s, 0 };

    int mpf = 1000 / fps;
    assert(1000%fps == 0);

    G.cbs.sim = cb_sim;
    G.cbs.eff = cb_eff;
    G.cbs.rec = cb_rec;

    G.time.mpf = mpf;
    T.wait = fps*P2P_WAIT;

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
#if 0
    LOCK();
    for (int i=0; i<P2P_MAX_NET; i++) {
        if (G.net[i].s != NULL) {
            G.net[i].s = NULL;
            SDLNet_TCP_Close(G.net[i].s);
        }
    }
    UNLOCK();
    pthread_mutex_destroy(&G.lock);
    for (int i=0; i<P2P_MAX_MEM; i++) {
        free(G.mem.his[i]);
    }
    SDLNet_Quit();
#endif
    printf("[%02d] tcks %d\n", G.me, T.tcks);
    printf("[%02d] fwds %d %d\n", G.me, T.fwds_s, T.fwds_i);
    printf("[%02d] baks %d %d\n", G.me, T.baks_s, T.baks_i);
    fflush(stdout);
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
        int now = G.time.tick + DELTA_TICKS;

        if (G.paks.n>0 && G.time.dif2==0 && (G.time.max>now || last>now)) {
            // i'm too much in the past, need to hurry
            int xxx = (G.time.max>now ? G.time.max : last);
if (G.time.max > now) {
    printf("!!!!\n");
    fflush(stdout);
}
            int dif = (xxx - G.time.tick - DELTA_TICKS) * G.time.mpf;
            G.time.dif2 = dif + 1000;
            G.time.mpf2 = G.time.mpf*1000 / (dif+1000);
printf("[%02d] FWD from=%d to=%d dif=%d/%d\n", G.me, G.time.tick, last, dif/G.time.mpf, dif);
//fflush(stdout);
#if 1 // paper instrumentation
            if (G.time.tick > T.wait) {
                if (!oldislate) {
//printf("[%02d] late=%d\n", G.me, G.time.tick);
//printf("[%02d] etal=%d\n", G.me, G.time.tick);
                    T.fwds_i++;
                    islate = 1;
                    T.fwds_s += (xxx - G.time.tick - DELTA_TICKS);
                }
            }
            //printf("[%02d] GOFWD=%d [%d = 2*%d*%d/1000]\n", G.me, dif, x, dif, G.time.mpf);
#endif
            break;
        } else if (G.paks.i==G.paks.n) {
            break;
        } else if (next == G.time.tick) {
            // i'm just in time, handle next event in real time
//printf("[%02d] TMR\n", G.me);
//fflush(stdout);
            G.cbs.sim(PAK(G.paks.i).evt);
            G.cbs.eff(0);
            G.paks.i++;
        } else if (next < G.time.tick) {
//printf("[%02d] BAK\n", G.me);
//fflush(stdout);
            // i'm in the future, need to go back and forth
            //int dt = MIN(G.time.mpf/2, 500/(G.time.tick-next)/2);
            G.paks.n--;     // do not include deviating event
#if 1 // paper instrumentation
            if (G.time.tick > T.wait) {
                T.baks_i += 1;
                T.baks_s += (G.time.tick-next);
            }
            //printf("[%02d] GOBAK=%d\n", G.me, G.time.tick-next);
#endif
#if 0
            for (int j=G.time.tick-1; j>next; j--) {
                p2p_travel(j);
                G.cbs.eff(1);
                //if (dt>0) SDL_Delay(dt);
            }
            G.paks.n++;     // now include it and move forward
            for (int j=next; j<=G.time.tick; j++) {
                p2p_travel(j);
                G.cbs.eff(1);
                //if (dt>0) SDL_Delay(dt);
            }
#endif
            G.paks.n++;     // now include it and move forward
            p2p_travel(G.time.tick);
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
        if (G.time.dif2 > 0) {
            G.time.dif2 -= G.time.mpf;
            G.time.nxt += G.time.mpf2;
        } else {
            G.time.nxt += G.time.mpf;
        }
        G.cbs.sim((p2p_evt) { P2P_EVT_TICK, 1, {.i1=G.time.tick} });
        if (G.time.tick % P2P_HIS_TICKS == 0) {
            assert(P2P_MAX_MEM > G.time.tick/P2P_HIS_TICKS);
            memcpy(G.mem.his[G.time.tick/P2P_HIS_TICKS], G.mem.app.buf, G.mem.app.n);    // save w/o events
        }
        if (G.time.tick > T.wait) {
            T.tcks++;
        }
        G.cbs.eff(0);
    }

    // SYNC
#if 1
    {
        static uint32_t sync = 0; //1000 / G.time.mpf;
        if (G.time.tick >= sync) {
//printf("????\n");
//fflush(stdout);
            p2p_pak pak = { G.me, G.time.tick, { P2P_EVT_SYNC, 0, {} }};
            p2p_bcast2(pak);
            sync += 1000 / G.time.mpf;
        }
    }
#endif

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

if (G.time.dif2 == 0) {
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
    }
    return 0;
}
