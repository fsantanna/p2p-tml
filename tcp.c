int tcp_recv_n (TCPsocket s, int n, char* buf) {
    int i = 0;
    while (i < n) {
        int n = SDLNet_TCP_Recv(s, &buf[i], n-i);
        if (n <= 0) {
            return 0;
        }
        i += n;
    }
    return 1;
}

int tcp_recv_u8 (TCPsocket s, uint8_t* ret) {
    return tcp_recv_n(s, sizeof(v), (char*)ret);
}

int tcp_recv_u32 (TCPsocket s, uint32_t* ret) {
    int ok = tcp_recv_n(s, sizeof(v), (char*)ret);
    *ret = be32toh(*ret);
    return ok;
}

int tcp_send_n (TCPsocket s, int n, char* buf) {
    return (SDLNet_TCP_Send(s,buf,n) == n);
    //int x = SDLNet_TCP_Send(s,buf,n);
    //printf(">>> %d, %s\n", x, SDLNet_GetError());
    //assert(x == n);
}

int tcp_send_u8 (TCPsocket s, uint8_t v) {
    return tcp_send_n(s, sizeof(v), (char*) &v);
}

int tcp_send_u32 (TCPsocket s, uint32_t v) {
    v = htobe32(v);
    return tcp_send_n(s, sizeof(v), (char*) &v);
}
