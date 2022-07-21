#include <stdint.h>
#include <SDL.h>

enum {
    TML_EVT_INIT = 0,
    TML_EVT_TICK,
    TML_EVT_NEXT
};

#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))

enum {
    TML_RET_NONE = 0,
    TML_RET_QUIT,
    TML_RET_REC,
    TML_RET_TRV
};

typedef struct {
    uint8_t id;
    uint8_t n;
    union {
        int i1;
        struct {
            int _1,_2;
        } i2;
        struct {
            int _1,_2,_3;
        } i3;
        struct {
            int _1,_2,_3,_4;
        } i4;
    } pay;
} p2p_evt;

typedef struct {
    char     status;    // 0=receiving, -1=repeated, 1=ok
    uint8_t  src;
    uint32_t seq;
    uint32_t tick;
    p2p_evt  evt;
} p2p_pak;

void tml_loop (
    int fps,                                // desired frame rate
    int mem_n,                              // memory size in bytes
    void* mem_buf,                          // pointer to memory contents
    void (*cb_sim) (p2p_evt),               // simulation callback
    void (*cb_eff) (int trv),               // effects callback
    int (*cb_rec) (SDL_Event*,p2p_evt*),    // event recording callback
    int (*cb_trv) (SDL_Event*,int,int,int*) // travel mode callback
);

void p2p_init  (uint8_t me, int port);
void p2p_quit  (void);
int  p2p_step  (p2p_evt* evt);
void p2p_bcast (uint32_t tick, p2p_evt* evt);
void p2p_link  (char* host, int port, uint8_t me);
void p2p_dump  (void);
