#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 4242
#define BUF_SIZE 2048
#define DEBUG_printf printf
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5
 
typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    int buffer_len;
    int sent_len;
    bool complete;
    int run_count;
    bool connected;
} TCP_CLIENT_T;

err_t tcp_client_close(void *arg);

err_t tcp_result(void *arg, int status);

err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) ;

err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);

void tcp_client_err(void *arg, err_t err);

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

bool tcp_client_open(void *arg);

TCP_CLIENT_T* tcp_client_init(void);

void run_tcp_client_test(TCP_CLIENT_T* state);

#endif