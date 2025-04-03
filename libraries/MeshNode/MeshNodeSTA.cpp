#include "MeshNode.hpp"
#include "Messages.hpp"
#include <lwip/tcpbase.h>
#include <stdio.h>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string.h>
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


#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#if 1
 static void dump_bytes(const void *ptr, uint32_t len) {
    const uint8_t* bptr = (uint8_t*)ptr;
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

#define BYTE_FROM_32BITS(INT, SHIFT) (((INT) >> ((SHIFT) * 8)) & 0xFF)
 

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
            printf("close failed %d, calling abort\n", err);
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
        printf("test success\n");
    } else {
        printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_client_close(arg);
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    //printf("tcp_client_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {
        printf("Successfully sent full %d byte message\n", BUF_SIZE);

        // We should receive a new buffer from the server
        state->buffer_len = 0;
        state->sent_len = 0;
        //printf("Waiting for buffer from server\n");
    }

    return ERR_OK;
}

// static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
//     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
//     printf("tcp_client_sent called, len=%d\n", len);
//     //DUMP_BYTES()

//     return ERR_OK;
// }

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    state->connected = true;
    //printf("Waiting for buffer from server\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("tcp_client_poll\n");
    return ERR_OK; // no response is an error?
}

static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    if (!p) {
        printf("No p_buffer detected, ending\n");
        return tcp_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
        state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                               p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);


    if (state->buffer_len == BUF_SIZE) {
        printf("\n\tReceived full %d buffer:\n", BUF_SIZE);
        if (state->buffer[1] == 0x00) {
            printf("Message priority: %02X\n", state->buffer[0]);
            uint32_t node_id = *reinterpret_cast<uint32_t*>(state->buffer + 2);
            printf("Recieved personal id: %04X\n", node_id);
        }
        else {
            printf("Message priority: %02X\n", state->buffer[0]);
            printf("Recieved data: %d\n\n", state->buffer[2]);
        }
    }

    return ERR_OK;
}

bool tcp_client_open(void *arg) {
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        printf("failed to create pcb\n");
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
        printf("failed to allocate state\n");
        return NULL;
    }
    ip4addr_aton("192.168.4.1", &state->remote_addr);
    return state;
}


// static void create_join_message(size_t buff_size, uint8_t* buff, STANode* node) {
//   buff[0] = 0xFF; // Highest priority message signaling incoming connection
//   buff[1] = 0x00; // Init message

//   *reinterpret_cast<uint32_t*>(buff + 2) = node->get_NodeID();
  
//   // send current downstream nodes connected to our hardware connected AP 
//   int connected_nodes = 0;
//   buff[6] = connected_nodes; // connected nodes
//   for (int i = 0; i < connected_nodes; i++)
//   {
//     buff[7+(i*4)] = 0;
//     buff[8+(i*4)] = 0;
//     buff[9+(i*4)] = 0;
//     buff[10+(i*4)] = 0;
//   }
// }




STANode::STANode() {
    upstream_node = ~0;
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

// Fix memory leak in scan_result
int STANode::scan_result(void* env, const cyw43_ev_scan_result_t* result) {
    if (result == NULL) {
        return 0;
    }

    // No need to allocate memory for result_copy if we're not storing it
    // Just use the result directly
    cyw43_ev_scan_result_t* result_copy = static_cast<cyw43_ev_scan_result_t*>(malloc(sizeof(cyw43_ev_scan_result_t)));
    *result_copy = *result;
    
    STANode* self = static_cast<STANode*>(env);
    const char* ssid_str = reinterpret_cast<const char*>(result->ssid);
    const char* prefix = "GatorGrid_Node:";
    size_t prefix_len = strlen(prefix);

    if (strncmp(ssid_str, prefix, prefix_len) == 0) {
        const char* incomingID = ssid_str + prefix_len;

        char* endptr;
        unsigned long value = strtoul(incomingID, &endptr, 16);
        uint32_t id = (uint32_t) value;
        // Add to known nodes if not already present
        
        if (self->known_nodes.find(id) == self->known_nodes.end()) {
            printf("New node ID: %u\n", id);
            printf("SSID: %-32s\n", result_copy->ssid);
            self->known_nodes[id] = result_copy;
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
            //cyw43_arch_poll();
            //cyw43_arch_wait_for_work_until(scan_time);
        #else
            sleep_ms(100);
        #endif
    }
    
    printf("Scan completed successfully\n");
    return true;
}

bool STANode::connect_to_node(uint32_t id) {
    if (upstream_node == ~0) {
        upstream_node = id;
    }

    printf("Connecting to node:%d\n", id);
    if (known_nodes.count(id) == 0) {
        printf("Unknown node. Ending connection attempt\n");
        return false;
    }
    printf("Connecting to %-32s\n", known_nodes.at(id)->ssid);
    if (cyw43_arch_wifi_connect_timeout_ms((char*)known_nodes.at(id)->ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        // for (int i = 0; i < 20; i ++)
        // {
            printf("failed to connect.\n");
        //     sleep_ms(1000);
        // }
        
        return false;
    } else {
        printf("Connected.\n");
    }
    
    return true;
}

bool STANode::is_connected() {
    int res = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    if (res != CYW43_LINK_JOIN) {
      return false;
    }
    return true;
}

bool STANode::tcp_init() {
    state = tcp_client_init();
    if (!state) {
        printf("Unable to initialize tcp client state\n");
        return false;
    }
    printf("Initialized tcp client\n");
    if (!tcp_client_open(this)) {
        printf("Failed to open client\n");
        tcp_result(state, -1);
        return false; 
    }
    printf("Opened tcp client connection\n");

    uint8_t buffer[BUF_SIZE] = {};
    //create_join_message(BUF_SIZE, buffer, this);
    TCP_INIT_MESSAGE init_msg(get_NodeID());
    DUMP_BYTES(init_msg.get_msg(), init_msg.get_len());
    return send_tcp_data(init_msg.get_msg(), init_msg.get_len());

}

bool STANode::send_tcp_data(uint8_t* data, uint32_t size) {

    uint8_t buffer[BUF_SIZE] = {};
    if (size > BUF_SIZE) { size = BUF_SIZE; }
    memcpy(buffer, data, size);

    bool flag = false;

    // while(state->tcp_pcb->snd_buf != 0) {
    //   printf("tcp buffer has %d bytes in it\n", state->tcp_pcb->snd_buf);
    // }

    cyw43_arch_lwip_begin();
    //printf("Space used in the buffer %d\n", tcp_sndbuf(state->tcp_pcb));
    // while (tcp_write(state->tcp_pcb, (void*)buffer, BUF_SIZE, TCP_WRITE_FLAG_COPY) == -1) {
    //     printf("attempting to write\n");
    //     sleep_ms(2);
    // }
    err_t err = tcp_write(state->tcp_pcb, (void*)buffer, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    err_t err2 = tcp_output(state->tcp_pcb);
    if (err != ERR_OK) {
        printf("Message failed to write\n");
        printf("ERR: %d\n", err);
        flag = true;
    }
    if (err2 != ERR_OK) {
        printf("Message failed to be sent\n");
        flag = true;
    }
    cyw43_arch_lwip_end();
    if (flag)
      return false;
    printf("Successfully queued message\n");
    return true;
}
