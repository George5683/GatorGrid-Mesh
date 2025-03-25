#include "MeshNode.hpp"
#include <pico/rand.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "pico/stdlib.h"
    
    #include "lwip/pbuf.h"
    #include "lwip/tcp.h"
    
    #include "dhcpserver.h"
    #include "dnsserver.h"

    #include "hardware/vreg.h"
    #include "hardware/clocks.h"

    #include "tcp_client.h"
    #include "tcp_server.h"
}


// APNode class constructor
APNode::APNode() : state(nullptr), running(false), password("password") {
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
    
}

// MeshNode class constructor
MeshNode::MeshNode(){
    // Seed the random number generator
    //std::srand(static_cast<unsigned>(time(nullptr)));
    seed_rand();
    generate_id();
}

// MeshNode deconstructor
MeshNode::~MeshNode(){
    // TODO - clean up AP and STA mode structures
    // TODO - Turn off AP and STA mode wifi setups
    NodeID = 0;
}

// set node ID from the APNode class
void APNode::set_node_id(int ID){
    // calls the base class function
    set_NodeID(ID);
}

// MeshNode base class function to set NodeID
void MeshNode::set_NodeID(int ID){
    // Assign ID to NodeID
    NodeID = ID; // Random number between 1 and 10,000
}

// APNode class function for getting the node id
int APNode::get_node_id(){
    return get_NodeID();
}

// MeshNode class function for getting Node ID
int MeshNode::get_NodeID(){
    return NodeID;
}

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

// Meshnode Random ID generator
void MeshNode::generate_id() {
    set_NodeID(rand());
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
    // Initialize the driver
    if (cyw43_arch_init()) {
        printf("failed to initialize cyw43 driver\n");
        return false;
    }

    num_buffers = 8;

    buffers = (uint8_t**)malloc(sizeof(uint8_t*)*num_buffers);

    for (int i = 0; i < num_buffers; i++) {
        buffers[i] = (uint8_t*)malloc(BUF_SIZE);
    }
    
    return true;
}

void APNode::set_ap_credentials(char *name, const char* pwd) {
    //memcpy(ap_name, name, sizeof(ap_name));
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

void APNode::server_test() {
    run_tcp_server_test(state);
}

void APNode::server_test() {
    run_tcp_server_test(state);
}

bool APNode::digest_recv_buffer(uint8_t *buf) {
    number_of_filled_buffers--;
    memcpy(buf, buffers[number_of_filled_buffers], BUF_SIZE);
    return true;
}

ssize_t APNode::recv_buffer_queue_len() {
    return number_of_filled_buffers;
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
/*
*   Client Class
*/


int STANode::scan_result(void* env, const cyw43_ev_scan_result_t* result) {
    if (result == NULL) {
        return 0;
    }

    cyw43_ev_scan_result_t* result_copy = static_cast<cyw43_ev_scan_result_t*>(malloc(sizeof(cyw43_ev_scan_result_t)));

    if (!result_copy) {
        return -1;
    }

    *result_copy = *result;

    const char* ssid_str = reinterpret_cast<const char*>(result->ssid);

    const char* prefix = "GatorGrid_Node:";
    size_t prefix_len = strlen(prefix);

    if (strncmp(ssid_str, prefix, prefix_len) == 0) {
        const char* incomingID = ssid_str + prefix_len;
        printf("SSID matches. Node data: \"%s\"\n", incomingID);

        char* endptr;
        unsigned long value = strtoul(incomingID, &endptr, 16);
        uint32_t id = (uint32_t) value;
        STANode* self = static_cast<STANode*>(env);
        self->known_nodes[id] = result_copy;
    }
    
    return 0;
}

STANode::STANode() {
    generate_id();
}
STANode::~STANode() {
    // TODO - Clean up STA mode data structures
}

bool STANode::init_sta_mode() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    return true;
}

bool STANode::start_sta_mode() {
    cyw43_arch_enable_sta_mode();
    return true; // ggwp
}

bool STANode::connect_to_AP(char* SSID, char* password, int ms_timeout) {
    return cyw43_arch_wifi_connect_timeout_ms(SSID, password, CYW43_AUTH_WPA2_AES_PSK, ms_timeout);
}

void STANode::client_test(){
    run_tcp_client_test(state);
}

void STANode::init_tcp() {
    state = tcp_client_init();
     if (!state) {
         return;
     }
}

bool STANode::scan_for_nodes() {
    bool scan_in_progress = false;
    absolute_time_t scan_start_time = get_absolute_time();
    absolute_time_t scan_time = make_timeout_time_ms(5000);
    
    cyw43_wifi_scan_options_t scan_options = { 0 };
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, this, STANode::scan_result);
    if (err != 0) {
        DEBUG_printf("Failed to start scan: %d\n", err);
        return false;
    }
    scan_in_progress = true;
    while (scan_in_progress) {
        if (!cyw43_wifi_scan_active(&cyw43_state)) {
            DEBUG_printf("Scan completed\n");
            scan_in_progress = false;
        }
        int64_t elapsed_us = absolute_time_diff_us(get_absolute_time(), scan_start_time);
        // wait max 5 secs
        if (elapsed_us > 5000 * 1000) {
            DEBUG_printf("Scan taking too long; aborting scan...\n");
            scan_in_progress = false;
            return false;
        }

        #if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
            cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
            cyw43_arch_wait_for_work_until(scan_time);
        #else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
            sleep_ms(100);
        #endif
    }


    return true;
}
