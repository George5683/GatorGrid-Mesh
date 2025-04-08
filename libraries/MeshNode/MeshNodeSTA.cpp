#include "MeshNode.hpp"
#include "Messages.hpp"
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

#define TCP_PORT 80
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
    STANode* node = (STANode*)arg;
    TCP_CLIENT_T *state = node->state;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
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
        
        state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len, copy_len, 0);
        
        // Print the received data for debugging
        // char received_data[256] = {0}; // Small buffer for displaying part of response
        // pbuf_copy_partial(p, received_data, p->tot_len > 255 ? 255 : p->tot_len, 0);
        // DEBUG_printf("Response: %s\n", received_data);
        
        // Acknowledge receipt of the data
        tcp_recved(tpcb, p->tot_len);
        
        // Consider this a success - we got a response
        // if (p->tot_len > 0) {
        //     pbuf_free(p);
        //     return tcp_result(arg, 0);
        // }
        bool ACK_flag = true;
        TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(state->buffer));
        if (!msg) {
            printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
            ACK_flag = false;
        } else {

            uint8_t msg_id = state->buffer[1];
            switch (msg_id) {
                case 0x00: {
                    TCP_INIT_MESSAGE* initMsg = static_cast<TCP_INIT_MESSAGE*>(msg);
                    //does stuff
                    printf("Received initialization message from node %u", initMsg->msg.source);
                    break;
                }
                case 0x01: {
                    TCP_DATA_MSG* dataMsg = static_cast<TCP_DATA_MSG*>(msg);
                    //does stuff
                    break;
                }
                case 0x02: {
                    TCP_DISCONNECT_MSG* discMsg = static_cast<TCP_DISCONNECT_MSG*>(msg);
                    //does stuff
                    break;
                }
                case 0x03: {
                    TCP_UPDATE_MESSAGE* updMsg = static_cast<TCP_UPDATE_MESSAGE*>(msg);
                    //does stuff
                    break;
                }
                case 0x04: {
                    TCP_ACK_MESSAGE* ackMsg = static_cast<TCP_ACK_MESSAGE*>(msg);
                    //does stuff
                    break;
                }
                case 0x05: {
                    TCP_NAK_MESSAGE* nakMsg = static_cast<TCP_NAK_MESSAGE*>(msg);
                    //does stuff
                    break;
                }
                default:
                    printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
                    ACK_flag = false;
                    // SEND NAK message
                    //TCP_NAK_MESSAGE nakMsg(node->get_NodeID(), msg_id ? msg_id : 0, 0);
                    break;
            }
        }

        if (ACK_flag){
            TCP_ACK_MESSAGE ackMsg(node->get_NodeID(), ackMsg.msg.msg_id, ackMsg.msg.len);
            node->send_tcp_data(ackMsg.get_msg(), ackMsg.get_len());
        } else {
            // TODO: Update for error handling
            // identify the source from sender and send back?
            TCP_NAK_MESSAGE nakMsg(node->get_NodeID(), 0, 0);
            node->send_tcp_data(nakMsg.get_msg(), nakMsg.get_len());
        }

        delete msg;
        
    
    }

    
    pbuf_free(p);
    return ERR_OK;
}

static bool tcp_client_open(void *arg) {
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
    //stdio_init_all();

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
    
    if (strncmp(ssid_str, prefix, prefix_len) == 0) {
        unsigned int id; // Changed to unsigned int for hexadecimal
        if (sscanf(ssid_str, "GatorGrid_Node:%x", &id) == 1) { // Changed format to %x for hexadecimal
            // Add to known nodes if not already present
            if (self->get_NodeID() == id) {
                goto free_mem;
            } else if (self->known_nodes.find(id) == self->known_nodes.end()) {
                printf("New node ID: 0x%x\n", id); // Changed format to 0x%x for hexadecimal output
                printf("New node ID: 0x%x, SSID: %s, Signal strength: %d dBm\n", id, (char*)&(result->ssid), result->rssi);
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

    //uint8_t buffer[BUF_SIZE] = {};
    //create_join_message(BUF_SIZE, buffer, this);
    TCP_INIT_MESSAGE init_msg(get_NodeID());
    DUMP_BYTES(init_msg.get_msg(), init_msg.get_len());
    return send_tcp_data(init_msg.get_msg(), init_msg.get_len());

}

bool STANode::send_tcp_data(uint8_t* data, uint32_t size) {

    // uint8_t buffer[size] = {};
    // if (size > BUF_SIZE) { size = size; }
    // memcpy(buffer, data, size);

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
    err_t err = tcp_write(state->tcp_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);
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


bool STANode::send_string_data(const char* data_string) {
    // Initialize the TCP client
    TCP_CLIENT_T *state = tcp_client_init();
    if (!state) {
        printf("Failed to initialize TCP client\n");
        return false;
    }

    // Open the TCP connection
    if (!tcp_client_open(state)) {
        printf("Failed to open TCP connection\n");
        tcp_client_close(state);
        free(state);
        return false;
    }

    // Wait for the connection to be established with timeout
    absolute_time_t connect_timeout = make_timeout_time_ms(5000); // 5 second timeout
    while (!state->connected) {
        if (absolute_time_diff_us(get_absolute_time(), connect_timeout) < 0) {
            printf("Connection timeout\n");
            tcp_client_close(state);
            free(state);
            return false;
        }
        
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(connect_timeout);
        #else
            sleep_ms(100);
        #endif
    }

    // Add the ID to the string data 
    char buffer[BUF_SIZE];
    uint32_t node_id = get_NodeID();
    int header_len = snprintf(buffer, BUF_SIZE, "ID:%u;DATA:", node_id);
    
    // Check if we have enough space for the header and data
    if (header_len < 0 || header_len + strlen(data_string) >= BUF_SIZE) {
        printf("Buffer too small for data with ID header\n");
        tcp_client_close(state);
        free(state);
        return false;
    }
    
    // Append the actual data after the ID header
    strcpy(buffer + header_len, data_string);
    
    // Use the combined buffer instead of the original data_string
    size_t data_len = strlen(buffer);
    if (data_len > BUF_SIZE) {
        printf("Data string is too long\n");
        tcp_client_close(state);
        free(state);
        return false;
    }

    // Copy the data into the buffer
    memcpy(state->buffer, buffer, data_len);
    state->buffer_len = data_len;

    printf("Sending data: %s (length: %d)\n", buffer, data_len);

    // Send the data
    cyw43_arch_lwip_begin();
    err_t err = tcp_write(state->tcp_pcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
    
    if (err != ERR_OK) {
        printf("Failed to write data %d\n", err);
        cyw43_arch_lwip_end();
        tcp_client_close(state);
        free(state);
        return false;
    }
    
    // Force data to be sent immediately
    err = tcp_output(state->tcp_pcb);
    if (err != ERR_OK) {
        printf("Failed to output data %d\n", err);
        cyw43_arch_lwip_end();
        tcp_client_close(state);
        free(state);
        return false;
    }
    cyw43_arch_lwip_end();

    // Wait for the data to be sent or timeout
    absolute_time_t send_timeout = make_timeout_time_ms(15000); // 15 second timeout
    while (!state->complete) {
        if (absolute_time_diff_us(get_absolute_time(), send_timeout) < 0) {
            printf("Send timeout\n");
            tcp_client_close(state);
            free(state);
            return false;
        }
        
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(send_timeout);
        #else
            sleep_ms(100);
        #endif
    }

    //printf("Data transmission completed successfully\n");
    
    // Clean up
    tcp_client_close(state);
    free(state);
    return true;
}
