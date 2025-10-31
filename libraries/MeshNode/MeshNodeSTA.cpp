#include "../UART/UART.hpp"
//#include "../SPI/SPI.hpp"
#include "MeshNode.hpp"
#include "Messages.hpp"
#include <cstdint>
#include <cyw43.h>
#include <cyw43_country.h>
#include <lwip/ip_addr.h>
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

    #include "lwip/dhcp.h"
    #include "lwip/tcp.h"
    #include "pico/time.h"
}

/*
*   Client Class
*/

#define TCP_PORT 4242

#define BUF_SIZE 2048

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5
 

// ---------- Arch-safe wrappers ----------
#if PICO_CYW43_ARCH_THREADSAFE_BACKGROUND
  #define CYW_BEGIN()  cyw43_arch_lwip_begin()
  #define CYW_END()    cyw43_arch_lwip_end()
#else
  #define CYW_BEGIN()  ((void)0)
  #define CYW_END()    ((void)0)
#endif

// ---------- TCP close helper ----------
static void tcp_close_safely(struct tcp_pcb **ppcb, void **papp_state) {
    if (!ppcb || !*ppcb) return;
    CYW_BEGIN();
    struct tcp_pcb *pcb = *ppcb;

    // Unhook callbacks
    tcp_arg(pcb, NULL);
    tcp_poll(pcb, NULL, 0);
    tcp_sent(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_err(pcb, NULL);

    err_t e = tcp_close(pcb);
    if (e != ERR_OK) {
        // If close cannot allocate, abort to free resources immediately
        tcp_abort(pcb);
    }
    *ppcb = NULL;
    CYW_END();

}

static bool wait_for_link_status(int target_status, uint32_t timeout_ms) {
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while (!time_reached(until)) {
        CYW_BEGIN();
        int ls = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        CYW_END();
        if (ls == target_status) return true;
        sleep_ms(100);
    }
    return false;
}

static bool wait_for_ip(uint32_t timeout_ms) {
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while (!time_reached(until)) {
        CYW_BEGIN();
        bool ok = netif_is_up(netif) && dhcp_supplied_address(netif);
        CYW_END();
        if (ok) return true;
        sleep_ms(100);
    }
    return false;
}

static const char* ls_str(int ls) {
    switch (ls) {
        case CYW43_LINK_UP:   return "UP";
        case CYW43_LINK_NOIP: return "NOIP";
        case CYW43_LINK_DOWN: return "DOWN";
        case CYW43_LINK_JOIN: return "JOIN";
        case CYW43_LINK_FAIL: return "FAIL";
        case CYW43_LINK_BADAUTH: return "BADAUTH";
        default: return "?";
    }
}

static void log_netif_state(const char* tag) {
    struct netif* n = &cyw43_state.netif[CYW43_ITF_STA];
    CYW_BEGIN();
    int  ls  = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    bool up  = netif_is_up(n);
    bool ip  = dhcp_supplied_address(n);
    bool scn = cyw43_wifi_scan_active(&cyw43_state);
    CYW_END();
    DEBUG_printf("[%s] link=%d(%s) up=%d ip=%d scan=%d\n", tag, ls, ls_str(ls), up, ip, scn);
}

int wifi_tcp_reconnect(struct tcp_pcb **ppcb,
                       void **papp_state /*ignored: pass NULL*/,
                       uint32_t node_id,
                       const char *pass,
                       uint32_t auth,
                       uint32_t connect_ms)
{
    absolute_time_t t0 = get_absolute_time();
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];

    char ssid[32];
    snprintf(ssid, sizeof(ssid), "GatorGrid_Node:%08X", node_id);

    DEBUG_printf("[reconnect] enter: ssid=\"%s\" auth=%lu to=%lu ms ppcb=%p *ppcb=%p state=%p\n",
                 ssid ? ssid : "(null)", (unsigned long)auth, (unsigned long)connect_ms,
                 (void*)ppcb, ppcb ? (void*)*ppcb : NULL, papp_state ? *papp_state : NULL);
    log_netif_state("pre");

    if (ppcb && *ppcb) {
        DEBUG_printf("[reconnect] closing tcp pcb=%p\n", (void*)*ppcb);
        tcp_close_safely(ppcb, NULL);
        DEBUG_printf("[reconnect] tcp pcb closed; *ppcb=%p\n", ppcb ? (void*)*ppcb : NULL);
    } else {
        DEBUG_printf("[reconnect] no pcb to close\n");
    }

    log_netif_state("before-hard-reset");

    // Disable STA mode
    DEBUG_printf("[reconnect] disable STA mode\n");
    cyw43_arch_disable_sta_mode();
    sleep_ms(100); // allow internal state machines to settle

    // clear DHCP/IP
    CYW_BEGIN();
    dhcp_release(netif);
    dhcp_stop(netif);
    netif_set_link_down(netif);
    netif_set_down(netif);
    netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
    CYW_END();
    log_netif_state("after-force-down");

    CYW_BEGIN();
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    CYW_END();
    sleep_ms(50);
    log_netif_state("after-leave");

    DEBUG_printf("[reconnect] re-enable STA mode\n");
    cyw43_arch_enable_sta_mode();
    CYW_BEGIN();
    cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);
    cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_STA, true, CYW43_COUNTRY_USA);
    CYW_END();
    sleep_ms(100);
    log_netif_state("after-bringup");

    int st = cyw43_arch_wifi_connect_timeout_ms(ssid, pass, auth, connect_ms);

    uint32_t connect_elapsed_ms = (uint32_t) to_ms_since_boot(get_absolute_time()) -
                                  (uint32_t) to_ms_since_boot(t0);
    DEBUG_printf("[reconnect] connect rc=%d after %lu ms\n",
                 st, (unsigned long)connect_elapsed_ms);

    if (st) {
        ERROR_printf("[reconnect] FAILED: rc=%d (UP=0, NOIP=1, DOWN=2, JOIN=3, FAIL=4, BADAUTH=5, ...)\n", st);
        log_netif_state("connect-failed");
        return st; // typically -2 or -1
    }

    bool ip_ok = wait_for_ip(5000);
    DEBUG_printf("[reconnect] wait_for_ip -> %d\n", ip_ok);
    log_netif_state("post-connect");
    if (!ip_ok) {
        ERROR_printf("[reconnect] DHCP did not supply address after connect\n");
        return -2;
    }

    uint32_t total_ms = (uint32_t) to_ms_since_boot(get_absolute_time()) -
                        (uint32_t) to_ms_since_boot(t0);
    DEBUG_printf("[reconnect] SUCCESS in %lu ms\n", (unsigned long)total_ms);
    return 0;
}


static err_t tcp_client_close(void *arg) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    err_t err = ERR_OK;
    DEBUG_printf("Called");
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
    free(state);
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
        ERROR_printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;

    DEBUG_printf("Recv'd data");
    
    // Check for connection closed or error
    if (!p) {
        ERROR_printf("Connection closed or error\n");
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
        ERROR_printf("Invalid SSID format: %s\n", ssid);
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
    auto st = cyw43_arch_wifi_connect_timeout_ms(ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 7500);
    if (st) {
        ERROR_printf("Response from wifi connect: %d", st);
        return false;
    }

    DEBUG_printf("Connected successfully.\n");
    parent = id;
    return true;
}

bool STANode::is_connected() {
    struct netif* n = &cyw43_state.netif[CYW43_ITF_STA];
    CYW_BEGIN();
    int  ls = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    bool up = netif_is_up(n);
    bool ip = dhcp_supplied_address(n);
    CYW_END();
    // "Fully online": associated AND interface up AND we have an IP
    return (ls == CYW43_LINK_UP) && up && ip;
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
    if (ERR_OK != send_tcp_data_blocking(init_msg.get_msg(), init_msg.get_len(), false)) {
        tcp_client_close(this);
        return false;
    }
    return true;
    
}

// Check for serial messages and if you have any parse them
void STANode::poll() {
    static int count = 0;
    if(uart.BufferReady()) {
        handle_serial_message(uart.getReadBuffer());
    }

    if (!is_connected()) {
        ERROR_printf("Detected that we are no longer connected");

        while(!runSelfHealing());

        DEBUG_printf("Successfully rejoined network");
        status = ERR_OK;
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

//Returns true if parent not found
bool STANode::selfHealingCheck(){

    scan_for_nodes();

    if(is_root || !is_connected()){
        return false;
    }

    if(known_nodes.find(parent) == known_nodes.end()){
        DEBUG_printf("Parent node not found during self-healing scan.\n");
        return true;
    } 

    else {
        DEBUG_printf("Parent node found during self-healing scan.\n");
        return false;
    }

}

void STANode::addChildrenToBlacklist(){
    vector<uint32_t> q;
    uint32_t children_id[4]; 
    uint8_t number_of_children;
    q.push_back(get_NodeID());
    while (q.size() > 0) {
        uint32_t curr_id = q[0];
        self_healing_blacklist.push_back(curr_id);
        q.erase(q.begin());
        bool f = tree.get_children(curr_id, children_id, number_of_children);
        if (f) {
            for (int i = 0; i < number_of_children; i++) {
                q.push_back(children_id[i]);
            }
        }
    }

    return;
}

bool STANode::runSelfHealing(){

    //Check if parent is still connected. If not, enter if statement
    if(selfHealingCheck() == true){

        bool foundParent = false;

        int16_t min_rssi = known_nodes.begin()->second->rssi;
        uint32_t min_node_id = known_nodes.begin()->first;
        DEBUG_printf("Known nodes size: %d", known_nodes.size());
        if (known_nodes.size() == 0) {
            DEBUG_printf("Cannot see any nodes while attempting to reconnect");
            return false;
        }


        uint8_t potentialNewParent;
        uint8_t childrenCount;
        uint32_t childrenArray[4];

        while(foundParent == false){

            //Need to further implement. is_root must be made to always be correct in other functions. is_child_of_root must also be implemented
            //to ensure that this logic works.
            if(is_child_of_root == true){
                DEBUG_printf("Node directly connected to root has lost connection to root. Transplanting new root node.\n");
                is_child_of_root = false;
                is_root = true;
            }

            //MUST MAKE SURE THE NODE DOES NOT CONNECT TO ITS OWN CHILDREN/GRANDCHILDREN/FURTHER DESCENDANTS
            addChildrenToBlacklist();

            for (const auto& node : known_nodes) {
                DEBUG_printf("Knows node %u with rssi %d", node.first, node.second->rssi);
                if ((node.second->rssi > min_rssi) && std::count(self_healing_blacklist.begin(), self_healing_blacklist.end(), node.first) == 0) {
                    min_rssi = node.second->rssi;
                    min_node_id = node.first;
                }
            }

            potentialNewParent = min_node_id;

            parent = potentialNewParent;

            struct tcp_pcb* pcb_local = (this->state) ? this->state->tcp_pcb : NULL;
            int st = wifi_tcp_reconnect(&pcb_local,
                            /*papp_state=*/NULL,         // <- do not free your app state here
                            parent, "password",
                            CYW43_AUTH_WPA2_AES_PSK,
                            30000);
            // if (!dhcp_supplied_address(netif)) { ERROR_printf("no IP yet\n"); }
            while (!tcp_init()) {
                ERROR_printf("Failed to init connection... Retrying");
            }
            
            foundParent = true;
            DEBUG_printf("Connected to new parent node: %u\n", parent);
            uint32_t node_id = get_NodeID();
            // tree.move_node(node_id, parent);
            // TODO: move_node needs to be fixed
            return true;
        }
        
    }

    return true;
}