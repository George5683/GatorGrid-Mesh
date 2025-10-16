#include "../UART/UART.hpp"
//#include "../SPI/SPI.hpp"
#include "MeshNode.hpp"
#include "Messages.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "pico/stdlib.h"
    
    #include "lwip/pbuf.h"
    #include "lwip/tcp.h"
    
    #include "dhcpserver.h"
    #include "dnsserver.h"

    #include "hardware/vreg.h"
    #include "hardware/clocks.h"
    #include "display.hpp"
}

/*
*   Client Class
*/

#define TCP_PORT 4242

#define BUF_SIZE 2048

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5
 

static err_t tcp_client_close(void *arg) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    err_t err = ERR_OK;
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        state->tcp_pcb = NULL;
    }
    return err;
}

// Called with results of operation
static err_t tcp_result(void *arg, int status) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    if (status == 0) {
        DEBUG_printf("Response back gotten\n");
    } else {
        DEBUG_printf("Did not receive a response back!\n");
    }
    state->complete = true;
    return tcp_client_close(arg);
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    DEBUG_printf("tcp_client_sent %u\n", len);

    DEBUG_printf("TCP_CLIENT_SENT DUMP BYTES\n");
    DUMP_BYTES(state->buffer, len);

    //state->sent_len += len;


    state->buffer_len = 0;
    state->sent_len = 0;

    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    if (err != ERR_OK) {
       DEBUG_printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    state->connected = true;
    DEBUG_printf("Waiting for response from server...\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    //DEBUG_printf("tcp_client_poll\n");
    return tcp_result(arg, -1); // no response is an error?
}

static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;

    DEBUG_printf("Recv'd data");
    
    // Check for connection closed or error
    if (!p) {
        DEBUG_printf("Connection closed or error\n");
        return tcp_result(arg, 0); // Consider this a success as we've got a response
    }
    
    // This method is callback from lwIP, so cyw43_arch_lwip_begin is not required
    cyw43_arch_lwip_check();
    
    if (p->tot_len > 0) {
        DEBUG_printf("Received %d bytes, err %d\n", p->tot_len, err);
        
        // Copy data from pbuf to our buffer
        const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
        uint16_t copy_len = p->tot_len > buffer_left ? buffer_left : p->tot_len;
        
        state->buffer_len += pbuf_copy_partial(p, state->buffer, copy_len, 0);
        
        // Print the received data for debugging
        // char received_data[256] = {0}; // Small buffer for displaying part of response
        // pbuf_copy_partial(p, received_data, p->tot_len > 255 ? 255 : p->tot_len, 0);
        // DEBUG_printf("Response: %s\n", received_data);
        
        // Acknowledge receipt of the data
        tcp_recved(tpcb, p->tot_len);
               
        node->handle_incoming_data(state->buffer, p->tot_len);
    
    }
    DEBUG_printf("Freeing buffer and returning");
    pbuf_free(p);
    return ERR_OK;
}

static bool tcp_client_open(void *arg) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
   DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
       DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, node);
    //tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);

    state->buffer_len = 0;

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

// Perform initialisation
static TCP_CLIENT_T* tcp_client_init(void) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    ip4addr_aton("192.168.4.1", &state->remote_addr);
    return state;
}

STANode::STANode() : rb(10), tree(0) {
    // Use hardware-based entropy sources if possible
    uint32_t ID = 0;
    
    // set the NodeID variable
    set_NodeID(ID);

   DEBUG_printf("Node ID is initially set to 0!\n");

    // Set SPI to be a slave 
    DEBUG_printf("starting uart\n");
    uart.picoUARTInit();
    DEBUG_printf("uart initalized\n");
    uart.picoUARTInterruptInit();
    DEBUG_printf("uart intterupts initalized\n");
    
    known_nodes.clear();
    parent = UINT32_MAX;
}

STANode::~STANode() {

    for (auto it = known_nodes.begin(); it != known_nodes.end(); it++)
    {
        free(it->second);
    } 
}

struct data STANode::digest_data() {
    return this->rb.digest();
}

bool STANode::init_sta_mode() {

    uint32_t AP_ID = 0;

    // Wait for the AP pico to send the first message
    
    DEBUG_printf("Waiting for ID from AP over UART\n");

    while(!uart.BufferReady()) {
        sleep_ms(10);
    }

    uint8_t *buffer = uart.getReadBuffer();

    AP_ID = *(uint32_t*)buffer;
    DEBUG_printf("ID char: %d\n", AP_ID);
    tree.edit_head(AP_ID);

    this->set_NodeID(AP_ID);

    DEBUG_printf("STA new ID is: %08x\n", get_NodeID());

    if (cyw43_arch_init()) {
       DEBUG_printf("failed to initialise\n");
        return false;
    }

     // Set power mode to high power


    if(cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM) != 0) {
        while(true) {
            DEBUG_printf("Failed to set power state\n");
            sleep_ms(1000);
        }
    }

   DEBUG_printf("Initiation to STA mode successful\n");
    return true;
}

bool STANode::start_sta_mode() {
    cyw43_arch_enable_sta_mode();
    DEBUG_printf("Starting STA mode\n");

    // tell AP that it has fully started
    uint8_t buf[300];
    *(uint32_t*)buf = this->get_NodeID();

    uart.sendMessage((char*)buf);
    
    return true; // ggwp
}

int extract_id(const char *ssid) {
    // Find the position of ':'
    char *id_str = strchr(ssid, ':'); 
    if (id_str && *(id_str + 1)) {
        id_str++; // Move past ':'

        // Print extracted string for debugging
        //printf("Extracted ID string: %s\n", id_str);

        // Convert it to an integer
        int id = strtol(id_str, NULL, 16); // Assuming it's hexadecimal
        //printf("Converted ID: %d (Hex to Decimal)\n", id);
        return id;
    } else {
       DEBUG_printf("Invalid SSID format: %s\n", ssid);
        return -1;
    }
}

// Fix memory leak in scan_result
int STANode::scan_result(void* env, const cyw43_ev_scan_result_t* result) {
    if (result == NULL) {
        return 0;
    }
    // copy the result
    cyw43_ev_scan_result_t* result_copy = static_cast<cyw43_ev_scan_result_t*>(malloc(sizeof(cyw43_ev_scan_result_t)));
    if (result_copy) {
        memcpy(result_copy, result, sizeof(cyw43_ev_scan_result_t));
    }
          
    STANode* self = static_cast<STANode*>(env);
    const char* ssid_str = reinterpret_cast<const char*>(result->ssid);
    const char* prefix = "GatorGrid_Node:";
    size_t prefix_len = strlen(prefix);
    
    //printf(ssid_str);
    //printf("\n");
    
    if (strncmp(ssid_str, prefix, prefix_len) == 0) {
        unsigned int id; // Changed to unsigned int for hexadecimal
        if (sscanf(ssid_str, "GatorGrid_Node:%x", &id) == 1) { // Changed format to %x for hexadecimal
            // Add to known nodes if not already present
            if (self->get_NodeID() == id) {
                //printf("Ignoring own AP node\n");
                goto free_mem;
            } else if (self->known_nodes.find(id) == self->known_nodes.end()) {
               DEBUG_printf("New node ID: 0x%x\n", id); // Changed format to 0x%x for hexadecimal output
               DEBUG_printf("New node ID: 0x%x, SSID: %s, Signal strength: %d dBm\n", id, (char*)&(result->ssid), result->rssi);
                self->known_nodes[id] = result_copy;
            } else {
                // Free if node is already known
                goto free_mem;
            }
        } else {
            // Free if parsing fails
            goto free_mem;
        }
    } else {
        // Free if SSID doesn't match prefix
        goto free_mem;
    }
     
    return 0;

free_mem:
    free(result_copy);
    return 0;
}

// Improve scan_for_nodes with better error handling
bool STANode::scan_for_nodes() {
    known_nodes.clear();
    // Check if a scan is already in progress
    if (cyw43_wifi_scan_active(&cyw43_state)) {
       DEBUG_printf("Scan already in progress, skipping\n");
        return false;
    }
    
    absolute_time_t scan_start_time = get_absolute_time();
    absolute_time_t scan_time = make_timeout_time_ms(5000);
    
    cyw43_wifi_scan_options_t scan_options = { 0 };
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, this, STANode::scan_result);
    if (err != 0) {
       DEBUG_printf("Failed to start scan: %d\n", err);
        return false;
    }
    
    DEBUG_printf("...");
    
    // Wait for scan to complete with timeout
    while (cyw43_wifi_scan_active(&cyw43_state)) {
        if (absolute_time_diff_us(scan_start_time, get_absolute_time()) > 5000000) {
           DEBUG_printf("Scan timeout\n");
            return false;
        }

        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(scan_time);
        #else
            sleep_ms(100);
        #endif
    }
    
    DEBUG_printf("---");
    return true;
}

bool STANode::connect_to_network() {
    if (known_nodes.size() == 0) return false;
    scan_for_nodes(); 
    DEBUG_printf("Knows some nodes\n");

    int16_t min_rssi = known_nodes.begin()->second->rssi;
    uint32_t min_node_id = known_nodes.begin()->first;

    for (const auto& node : known_nodes) {
        if (node.second->rssi > min_rssi) {
            min_rssi = node.second->rssi;
            min_node_id = node.first;
        }
    }

    char ssid[32];  // Allocate on stack instead of heap
    snprintf(ssid, sizeof(ssid), "GatorGrid_Node:%08X", min_node_id);  // Convert ID to uppercase hex

    DEBUG_printf("Generated SSID: %s\n", ssid);

    // Attempt to connect
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        for (int i = 0; i < 20; i++) {
           DEBUG_printf("Failed to connect. Retrying...\n");
            sleep_ms(1000);  
        }
        return false;
    }

    DEBUG_printf("Connected successfully.\n");
    parent = min_node_id;
    return true;
}

bool STANode::connect_to_node(uint32_t id) {
   DEBUG_printf("Connecting to node: %u\n", id);

    if(!scan_for_nodes()) {
        ERROR_printf("Scan for nodes failed");
        return false;
    }

    // Check if node is known
    if (known_nodes.count(id) == 0) {
        DEBUG_printf("Unknown node. Ending connection attempt.\n");
        return false;
    }

    // Generate SSID string
    char ssid[32];  // Allocate on stack instead of heap
    snprintf(ssid, sizeof(ssid), "GatorGrid_Node:%08X", id);  // Convert ID to uppercase hex

    DEBUG_printf("Generated SSID: %s\n", ssid);

    // Attempt to connect
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 7500)) {
        for (int i = 0; i < 8; i++) {
           DEBUG_printf("Failed to connect. Retrying...\n");
            sleep_ms(1000);  
        }
        return false;
    }

    DEBUG_printf("Connected successfully.\n");
    parent = id;
    return true;
}

bool STANode::is_connected() {
    int res = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    if (res != CYW43_LINK_JOIN) {
        //DEBUG_printf("%d\n", res);
      return false;
    }
    return true;
}

bool STANode::tcp_init() {
    state = tcp_client_init();
    if (!state) {
       DEBUG_printf("Unable to initialize tcp client state\n");
        return false;
    }
    DEBUG_printf("Initialized tcp client\n");
    state->got_nak = false;
    if (!tcp_client_open(this)) {
       DEBUG_printf("Failed to open client\n");
        tcp_result(state, -1);
        return false; 
    }
    DEBUG_printf("Opened tcp client connection\n");

    //uint8_t buffer[BUF_SIZE] = {};
    //create_join_message(BUF_SIZE, buffer, this);
    TCP_INIT_MESSAGE init_msg(get_NodeID(), parent);
    return send_tcp_data_blocking(init_msg.get_msg(), init_msg.get_len(), false);
}

// Check for serial messages and if you have any parse them
void STANode::poll() {
    if(uart.BufferReady()) {
        handle_serial_message(uart.getReadBuffer());
    }
}

int STANode::number_of_messages() {
    return this->rb.get_size();
}

err_t STANode::update_network() {
    uint32_t children_ids[4] = {0};
    uint8_t children_count = 0;

    tree.get_children(get_NodeID(), children_ids, children_count);
    for (int i = 0; i < children_count; i++) {
        TCP_FORCE_UPDATE_MESSAGE forceUpdateMsg(children_ids[i], get_NodeID());
        send_msg(forceUpdateMsg.get_msg());
    }

    return 0;
}
