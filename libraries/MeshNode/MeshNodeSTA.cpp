#include "MeshNode.hpp"
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
}

/*
*   Client Class
*/

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 2048

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#if 0
 static void dump_bytes(const uint8_t *bptr, uint32_t len) {
     unsigned int i = 0;
 
     printf("dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
             printf("\n");
         } else if ((i & 0x07) == 0) {
             printf(" ");
         }
         printf("%02x ", bptr[i++]);
     }
     printf("\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif
 

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

static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
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
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_client_close(arg);
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("tcp_client_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        state->run_count++;
        if (state->run_count >= TEST_ITERATIONS) {
            tcp_result(arg, 0);
            return ERR_OK;
        }

        // We should receive a new buffer from the server
        state->buffer_len = 0;
        state->sent_len = 0;
        DEBUG_printf("Waiting for buffer from server\n");
    }

    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    state->connected = true;
    DEBUG_printf("Waiting for buffer from server\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_client_poll\n");
    return tcp_result(arg, -1); // no response is an error?
}

static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (!p) {
        return tcp_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DUMP_BYTES(q->payload, q->len);
        }
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
        state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                               p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // If we have received the whole buffer, send it back to the server
    if (state->buffer_len == BUF_SIZE) {
        DEBUG_printf("Writing %d bytes to server\n", state->buffer_len);
        err_t err = tcp_write(tpcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            DEBUG_printf("Failed to write data %d\n", err);
            return tcp_result(arg, -1);
        }
    }
    return ERR_OK;
}

static bool tcp_client_open(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
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

STANode::STANode() {
    
}

STANode::~STANode() {

    for (auto it = known_nodes.begin(); it != known_nodes.end(); it++)
    {
        free(it->second);
    } 
}

bool STANode::init_sta_mode() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return false;
    }

    printf("Initiation to STA mode successful\n");
    return true;
}

bool STANode::start_sta_mode() {
    cyw43_arch_enable_sta_mode();
    printf("Starting STA mode\n");
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
        printf("Invalid SSID format: %s\n", ssid);
        return -1;
    }
}

// Fix memory leak in scan_result
int STANode::scan_result(void* env, const cyw43_ev_scan_result_t* result) {
    if (result == NULL) {
        return 0;
    }

    // No need to allocate memory for result_copy if we're not storing it
    // Just use the result directly
    cyw43_ev_scan_result_t* result_copy = static_cast<cyw43_ev_scan_result_t*>(malloc(sizeof(cyw43_ev_scan_result_t)));
    
    STANode* self = static_cast<STANode*>(env);
    const char* ssid_str = reinterpret_cast<const char*>(result->ssid);
    const char* prefix = "GatorGrid_Node:";
    size_t prefix_len = strlen(prefix);

    if (strncmp(ssid_str, prefix, prefix_len) == 0) {
        int id;
        if (sscanf(ssid_str, "GatorGrid_Node:%d", &id) == 1) {
            id = extract_id(ssid_str);
            // Add to known nodes if not already present
            if (self->known_nodes.find(id) == self->known_nodes.end()) {
                printf("New node ID: %d\n", id);
                printf("New node ID: %d, SSID: %s, Signal strength: %d dBm\n", id, (char*)&(result->ssid), result->rssi);
                self->known_nodes[id] = result_copy;
            }
        }
    }
    else
    {
        free(result_copy);
    }
    
    return 0;
}

// Improve scan_for_nodes with better error handling
bool STANode::scan_for_nodes() {
    // Check if a scan is already in progress
    if (cyw43_wifi_scan_active(&cyw43_state)) {
        printf("Scan already in progress, skipping\n");
        return false;
    }
    
    absolute_time_t scan_start_time = get_absolute_time();
    absolute_time_t scan_time = make_timeout_time_ms(5000);
    
    cyw43_wifi_scan_options_t scan_options = { 0 };
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, this, STANode::scan_result);
    if (err != 0) {
        printf("Failed to start scan: %d\n", err);
        return false;
    }
    
    printf("Scan started\n");
    
    // Wait for scan to complete with timeout
    while (cyw43_wifi_scan_active(&cyw43_state)) {
        if (absolute_time_diff_us(scan_start_time, get_absolute_time()) > 5000000) {
            printf("Scan timeout\n");
            return false;
        }

        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(scan_time);
        #else
            sleep_ms(100);
        #endif
    }
    
    printf("Scan completed successfully\n");
    return true;
}

bool STANode::connect_to_node(uint32_t id) {
    printf("Connecting to node: %u\n", id);

    // Check if node is known
    if (known_nodes.count(id) == 0) {
        printf("Unknown node. Ending connection attempt.\n");
        return false;
    }

    // Generate SSID string
    char ssid[32];  // Allocate on stack instead of heap
    snprintf(ssid, sizeof(ssid), "GatorGrid_Node:%08X", id);  // Convert ID to uppercase hex

    printf("Generated SSID: %s\n", ssid);

    // Attempt to connect
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        for (int i = 0; i < 20; i++) {
            printf("Failed to connect. Retrying...\n");
            sleep_ms(1000);  
        }
        return false;
    }

    printf("Connected successfully.\n");
    return true;
}


