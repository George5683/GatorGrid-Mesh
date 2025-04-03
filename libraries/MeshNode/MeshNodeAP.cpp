#include "MeshNode.hpp"
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "pico/stdlib.h"
    
    #include "lwip/pbuf.h"
    #include "lwip/tcp.h"
    
    #include "dhcpserver.h"
    #include "dnsserver.h"

    #include "hardware/vreg.h"
    #include "hardware/clocks.h"
}

// TCP Server state structures
typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[128];
    char result[256];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    void* ap_node;
} TCP_CONNECT_STATE_T;

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
    int sent_len;
    int recv_len;
    int run_count;
    ip_addr_t gw;
    void* ap_node;
    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
} TCP_SERVER_T;
 


TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_server_close(arg);
}

err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    /*for(int i=0; i< BUF_SIZE; i++) {
        state->buffer_sent[i] = rand();
    }*/

    state->sent_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n", BUF_SIZE);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        return tcp_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);

        if(state->recv_len == 2048) {
            printf("SERVER DEBUG: GOT 2048\n");
            for(int i=0; i < BUF_SIZE; i++) {
                state->buffer_sent[i] = state->buffer_recv[i];
            }
            tcp_server_send_data(arg, state->client_pcb);

            for (int i = 0; i < 4; i++) {
                printf("recv buff[%d] == %02x\n", i, state->buffer_recv[i]);
                printf("sent buff[%d] == %02x\n", i, state->buffer_sent[i]);
            }

        } else {
            printf("SERVER DEBUG: Currently recv %d\n", state->recv_len);
        }
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE) {

        // check it matches
        if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0) {
            DEBUG_printf("buffer mismatch\n");
            return tcp_server_result(arg, -1);
        }
        DEBUG_printf("tcp_server_recv buffer ok\n");

        // Test complete?
        //state->run_count++;
        /*if (state->run_count >= TEST_ITERATIONS) {
            tcp_server_result(arg, 0);
            return ERR_OK;
        }*/

        // Send another buffer
        return ERR_OK;
        //return tcp_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return ERR_OK;
    //return tcp_server_result(arg, -1); // no response is an error?
}

void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    //return tcp_server_send_data(arg, state->client_pcb);
    return ERR_OK;
}

bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void run_tcp_server_test(TCP_SERVER_T *state) {

    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    while(!state->complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    free(state);
}


// APNode class constructor
APNode::APNode() : state(nullptr), running(false), password("password"), webpage_enabled(false) {
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
    
}



// set node ID from the APNode class
void APNode::set_node_id(int ID){
    // calls the base class function
    set_NodeID(ID);
}

// MeshNode base class function to set NodeID
void MeshNode::set_NodeID(uint32_t ID){
    // Assign ID to NodeID
    NodeID = ID; // Random number between 1 and 10,000
}

// APNode class function for getting the node id
int APNode::get_node_id(){
    return get_NodeID();
}

// MeshNode class function for getting Node ID
uint32_t MeshNode::get_NodeID(){
    return NodeID;
}

// APNode deconstructor
APNode::~APNode(){
    // Make sure AP mode is stopped
    stop_ap_mode();
    
    // Free the state if it exists
    if (state) {
        free(state);
        state = nullptr;
    }
    
    // Deinitialize the driver
    cyw43_arch_deinit();
}

bool APNode::init_ap_mode() {
    // Allocate the state of the TCP server if not already allocated
    if (!state) {
        state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
        if (!state) {
            DEBUG_printf("failed to allocate state\n");
            return false;
        }
    }
    
    // Initialize the state
    state->complete = false;
    state->server_pcb = nullptr;
    state->ap_node = this;
    
    // Initialize the driver
    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialize cyw43 driver\n");
        return false;
    }
    
    return true;
}

void APNode::set_ap_credentials(char name[32], const char* pwd) {
    memcpy(ap_name, name, sizeof(ap_name));
    password = pwd;
}

void APNode::poll(unsigned int timeout_ms) {
    if (!running || !state) {
        return;
    }
    
    // Check if we should stop
    if (state->complete) {
        running = false;
        return;
    }
    
    // Handle polling operations based on the architecture
    #if PICO_CYW43_ARCH_POLL
    // If using poll architecture, check for work
    cyw43_arch_poll();
    // Wait for work or timeout
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(timeout_ms));
    #else
    // If using interrupt architecture, just sleep
    sleep_ms(timeout_ms);
    #endif
}

void APNode::stop_ap_mode() {
    if (!running) {
        return;
    }
    
    // Disable the AP mode
    cyw43_arch_lwip_begin();
    cyw43_arch_disable_ap_mode();
    cyw43_arch_lwip_end();
    
    // Close the TCP server
    if (state && state->server_pcb) {
        tcp_server_close(state);
    }
    
    running = false;
}

bool APNode::start_ap_mode() {

    // Enable the AP mode on the driver
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    state = tcp_server_init();
    if (!state) {
        return false;
    }

    #if LWIP_IPV6
     #define IP(x) ((x).u_addr.ip4)
     #else
     #define IP(x) (x)
     #endif
 \
     ip4_addr_t mask;
     IP(state->gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
     IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
 
     #undef IP

    // Start the DHCP server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    server_test();
    
    running = true;
    return true;
}

void APNode::server_test() {
    run_tcp_server_test(state);
}

// MeshNode class constructor
MeshNode::MeshNode() {
    // Use hardware-based entropy sources if possible
    uint32_t ID = 0;
    
    // Generate a seed using multiple entropy sources
    uint64_t seed = time(nullptr);
    seed ^= (uint64_t)this;  // Add pointer address as entropy
    
    // Mix in hardware-specific entropy if available
    #if defined(PICO_UNIQUE_BOARD_ID_SIZE_BYTES)
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
        seed = seed * 33 + board_id.id[i];
    }
    #endif
    
    // Use a better random generator
    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);
    
    ID = dist(rng);
    
    // set the NodeID variable
    set_NodeID(ID);
    
    printf("Generated NodeID: %u\n", ID);
}

// MeshNode deconstructor
MeshNode::~MeshNode(){
    NodeID = 0;
}