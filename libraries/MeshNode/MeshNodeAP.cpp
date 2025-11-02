#include "MeshNode.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "display.hpp"

 #define TRY(x)            \
 do                        \
 {                         \
     err_t st;             \
     st = (x);             \
     if (st != 0)          \
         return st;        \
 } while (0)


// https://forums.raspberrypi.com/viewtopic.php?t=302960
void MeshNode::seed_rand() {

    uint32_t random = 0x811c9dc5;
    uint8_t next_byte = 0;
    volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
  
    for (int i = 0; i < 16; i++) {
      for (int k = 0; k < 8; k++) {
        next_byte = (next_byte << 1) | (*rnd_reg & 1);
      }
  
      random ^= next_byte;
      random *= 0x01000193;
    }
  
    srand(random);
  }

TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

// this callback is bad 
err_t tcp_server_close(void *arg) {
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        DEBUG_printf("closing client_pcb!");
        if(node->clients_map.find(state->client_pcb) != node->clients_map.end()) {
            DEBUG_printf("Node ID Found %d\n", node->clients_map[state->client_pcb].id);
        } else {
            DEBUG_printf("Node ID Not Found\n");
        }
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        DEBUG_printf("Close called\n");
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        DEBUG_printf("closing server_pcb! This should not called!\n");
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }

    DEBUG_printf("Function returning\n");
    return err;
}

err_t tcp_server_result(void *arg, int status) {
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_server_close(arg);
}

err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;
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
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;
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
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;

    //APNode* node = (APNode*)state->ap_node;

    bool got_full_message = false;

    DEBUG_printf("Recv called\n");
    if (!p) {
        ERROR_printf("pbuf returned null");
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
        //printf("before pbuf copy partial\n");
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        //printf("after pbuf copy partial\n");
        tcp_recved(tpcb, p->tot_len);
        //printf("before tcp_recved\n");

        if(state->recv_len == 2048) {
           DEBUG_printf("SERVER DEBUG: GOT 2048\n");
            //got_full_message = true;

        } else {
           DEBUG_printf("SERVER DEBUG: Currently recv %d\n", state->recv_len);
        }

        if(node->clients_map.find(tpcb) != node->clients_map.end()) {
           DEBUG_printf("PCB FOUND\n");
        } else {
           DEBUG_printf("MAJOR ERROR CLIENT PCB NOT FOUND\n");
            return ERR_OK; // TODO: Change error handling
        }

        node->handle_incoming_data(state->buffer_recv, tpcb, p, node);

        //tcp_server_send_data(arg, state->client_pcb);
        // for (int i = 0; i < 20; i++) {
        //    DEBUG_printf("recv buff[%d] == %02x\n", i, state->buffer_recv[i]);
           
        // }
    }

    // Reset the TCP buffer state or else data will just be appened to the state recv buffer
    state->recv_len = 0;

    pbuf_free(p);

    // Have we have received the whole buffer
    // if (state->recv_len == BUF_SIZE) {

    //     // check it matches
    //     if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0) {
    //         DEBUG_printf("buffer mismatch\n");
    //         return tcp_server_result(arg, -1);
    //     }
    //     DEBUG_printf("tcp_server_recv buffer ok\n");

    //     // Test complete?
    //     //state->run_count++;
    //     /*if (state->run_count >= TEST_ITERATIONS) {
    //         tcp_server_result(arg, 0);
    //         return ERR_OK;
    //     }*/

    //     // Send another buffer
    //     return ERR_OK;
    //     //return tcp_server_send_data(arg, state->client_pcb);
    // }
    return ERR_OK;
}

err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return ERR_OK;
    //return tcp_server_result(arg, -1); // no response is an error?
}

void tcp_server_err(void *arg, err_t err) {
    ERROR_printf("Called with error %d", err);
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_server_err Called!!!\n");
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    APNode* node = (APNode*)state->ap_node;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    struct ClientConnection client;

    client.pcb = client_pcb;

    node->clients.push_back({client});

    node->clients_map.insert({client_pcb, client});

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, node);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    // tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
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

bool APNode::server_running() {
    return !this->state->complete;
}

void run_tcp_server(void * arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
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
   DEBUG_printf("Now freeing state\n");
    free(state);
    
}

// APNode class constructor
APNode::APNode() : state(nullptr), running(false), password("password"), rb(10), tree(get_NodeID())  {
        // initalize uart

    DEBUG_printf("starting uart\n");
    uart.picoUARTInit();
    DEBUG_printf("uart  nitalized\n");
    uart.picoUARTInterruptInit();
    DEBUG_printf("uart intterupts initalized\n");
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
}

APNode::APNode(uint32_t id) : state(nullptr), running(false), password("password"), rb(10), tree(id) {
        // initalize uart

    

    DEBUG_printf("starting uart\n");
    uart.picoUARTInit();
    DEBUG_printf("uart initalized\n");
    uart.picoUARTInterruptInit();
    DEBUG_printf("uart intterupts initalized\n");
    set_NodeID(id);
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
}

struct data APNode::digest_data() {
    return this->rb.digest();
}

int APNode::number_of_messages() {
    return this->rb.get_size();
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

bool MeshNode::get_is_root() {
    return is_root;
}
void MeshNode::set_is_root(bool status) {
    is_root = status;
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

    state->recv_len = 0;
    
    // Initialize the driver
    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialize cyw43 driver\n");
        return false;
    }

     // Set power mode to high power


    if(cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM) != 0) {
        while(true) {
            DEBUG_printf("Failed to set power state\n");
            sleep_ms(1000);
        }
    }
    
    return true;
}

void APNode::set_ap_credentials(char name[32], const char* pwd) {
    memcpy(ap_name, name, sizeof(ap_name));
    password = pwd;
}

void APNode::poll(unsigned int timeout_ms) {
    if(uart.BufferReady()) {
        handle_serial_message(uart.getReadBuffer());
    }
    int st = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_AP);
    if (st == CYW43_LINK_NONET || st == CYW43_LINK_DOWN || st == CYW43_LINK_FAIL) {
        ERROR_printf("[WIFI] AP lost, status=%d\n", st);
        cyw43_wifi_leave(&cyw43_state, CYW43_ITF_AP);
        // Clean up sockets, stop services, and start reconnect logic…
        // e.g., cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
        // then try cyw43_arch_wifi_connect_timeout_ms(...) with backoff
        // runSelfHealing();
    }
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
        tcp_server_close(this);
    }
    
    running = false;
}

bool APNode::start_ap_mode() {

    // Delay to ensure the masters starts after the slave
    sleep_ms(2000);

    // Enable the AP mode on the driver
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    //state = tcp_server_init();
    if (!state) {
        return false;
    }

    #if LWIP_IPV6
     #define IP(x) ((x).u_addr.ip4)
     #else
     #define IP(x) (x)
     #endif
     
     ip4_addr_t mask;
     IP(state->gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
     IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
 
     #undef IP

    // Start the DHCP server
    //dhcp_server_t dhcp_server;
    dhcp_server_init(&state->dhcp_server, &state->gw, &mask);
    //dns_server_init(&state->dns_server, &state->gw);

    // if (!tcp_server_open(state)) {
    //     DEBUG_printf("failed to open server\n");
    //     return false;
    // }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return false;
    }

    // Start SPI
    // Master_Pico.SPI_init();

    // uint32_t ID = this->get_node_id();
    // vector<uint8_t> temp =  { 'A', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
    //                                           'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
    //                                           'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
    //                                           'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f' };
    // *(uint32_t*)temp.data() = ID;

    // Master_Pico.SPI_send_message(temp);

    uint32_t ID = this->get_node_id();

    DEBUG_printf("Sending ID %d\n", ID);

    uart.sendMessage((char*)&ID);
    

    //DEBUG_printf("Message sent");
    //puts("entering poll test");

    //while(!Master_Pico.SPI_POLL_MESSAGE());

    // wait for sta to fully start
    while(!uart.BufferReady()) {
        sleep_ms(10);
    }

    uint8_t *tmp = uart.getReadBuffer();

    if(*(uint32_t *)tmp != this->get_NodeID()) {
        while(1) {
            printf("ERROR: Node ID DIFFERENT, This should not ever be hit");
        }
    }

    running = true;
    return true;
}

void APNode::server_start() {
    run_tcp_server(state);
}

// MeshNode class constructor
MeshNode::MeshNode() {
    seed_rand();
    
    // set the NodeID variable
    set_NodeID(rand());
}

// MeshNode deconstructor
MeshNode::~MeshNode(){
    NodeID = 0;
}
