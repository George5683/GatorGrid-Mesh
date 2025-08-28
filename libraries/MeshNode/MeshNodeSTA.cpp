#include "libraries/SPI/SPI.hpp"
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

#define DEBUG 0

#define TCP_PORT 4242
#if DEBUG 
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif
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
    volatile bool waiting_for_ack;
    volatile bool got_nak;
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

    DEBUG_printf("TCP_CLIENT_SENT DUMP BYTES\n");
    DUMP_BYTES(state->buffer, 100);

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
               
        node->handle_incoming_data(state->buffer, p);
    
    }
    
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

STANode::STANode() : rb(10) {
    // Use hardware-based entropy sources if possible
    uint32_t ID = 0;
    
    // set the NodeID variable
    set_NodeID(ID);

   DEBUG_printf("Node ID is initially set to 0!\n");

    // Set SPI to be a slave 
    Slave_Pico.SPI_init(false);
    known_nodes.clear();
    parent = 0xFFFFFFFF;
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
    //stdio_init_all();

    uint32_t AP_ID = 0;

    while(!Slave_Pico.SPI_is_read_available());

    Slave_Pico.SPI_read_message((uint8_t*)&AP_ID, 5);

    this->set_NodeID(AP_ID);

   DEBUG_printf("STA new ID is: %08x\n", get_NodeID());

    if (cyw43_arch_init()) {
       DEBUG_printf("failed to initialise\n");
        return false;
    }

   DEBUG_printf("Initiation to STA mode successful\n");
    return true;
}

bool STANode::start_sta_mode() {
    cyw43_state.ap_channel = 6;
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_pm(&cyw43_state, CYW43_DEFAULT_PM & ~0xf);
    DEBUG_printf("Starting STA mode\n");
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
    
    DEBUG_printf("Scan started");
    
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
    
    DEBUG_printf("Scan completed successfully");
    return true;
}

bool STANode::connect_to_node(uint32_t id) {
   DEBUG_printf("Connecting to node: %u\n", id);

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
    //if (cyw43_arch_wifi_connect_timeout_ms(ssid, "password", CYW43_AUTH_WPA2_AES_PSK, 20000)) {
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, NULL, CYW43_AUTH_OPEN, 20000)) {
        for (int i = 0; i < 20; i++) {
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
    TCP_INIT_MESSAGE init_msg(get_NodeID());
    return send_tcp_data(init_msg.get_msg(), init_msg.get_len(), false);

}

int STANode::number_of_messages() {
    return this->rb.get_size();
}

bool STANode::send_tcp_data(uint8_t* data, uint32_t size, bool forward) {

    // uint8_t buffer[size] = {};
    // if (size > BUF_SIZE) { size = size; }
    // memcpy(buffer, data, size);

    bool flag = false;

    // while(state->tcp_pcb->snd_buf != 0) {
    //  DEBUG_printf("tcp buffer has %d bytes in it\n", state->tcp_pcb->snd_buf);
    // }
    if (!forward) {
        while(state->waiting_for_ack); //{sleep_ms(5);}
    }
    //sleep_ms(10);
    cyw43_arch_lwip_begin();
    //printf("Space used in the buffer %d\n", tcp_sndbuf(state->tcp_pcb));
    // while (tcp_write(state->tcp_pcb, (void*)buffer, BUF_SIZE, TCP_WRITE_FLAG_COPY) == -1) {
    //    DEBUG_printf("attempting to write\n");
    //     sleep_ms(2);
    // }

    // Not getting an ack message back for some reason
    //sleep_ms(125);
    
    DEBUG_printf("Before tpc_write\n");

    err_t err = tcp_write(state->tcp_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);

    DEBUG_printf("After tpc_write\n");

    err_t err2 = tcp_output(state->tcp_pcb);

    DEBUG_printf("After tpc_output\n");


    if (err != ERR_OK) {
       DEBUG_printf("Message failed to write\n");
       DEBUG_printf("ERR: %d\n", err);
        flag = true;
    }
    if (err2 != ERR_OK) {
       DEBUG_printf("Message failed to be sent\n");
        flag = true;
    }
    cyw43_arch_lwip_end();
    if (flag)
      return false;
    
    DEBUG_printf("SENDING BYTES BELOW: \n");
    state->waiting_for_ack = !forward;
    DUMP_BYTES(data, size);
    return true;
}



/**
 * @brief Will wait until availble to send tcp, then will wait for an ACK response before releasing
 * 
 * @param data 
 * @param size 
 * @param forward 
 * @return true - message successfully sent
 * @return false - message failed to send
 */

bool STANode::send_tcp_data_blocking(uint8_t* data, uint32_t size, bool forward) {

    bool flag = false;

    while(state->waiting_for_ack); 
    cyw43_arch_lwip_begin();

    DEBUG_printf("blocking before write/output\n");
    err_t err = tcp_write(state->tcp_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);
    err_t err2 = tcp_output(state->tcp_pcb);
    DEBUG_printf("blocking after write/output\n");
    if (err != ERR_OK) {
       DEBUG_printf("Message failed to write\n");
       DEBUG_printf("ERR: %d\n", err);
        flag = true;
    }
    if (err2 != ERR_OK) {
       DEBUG_printf("Message failed to be sent\n");
        flag = true;
    }
    cyw43_arch_lwip_end();
    if (flag) {
        DEBUG_printf("Some error in write/output\n");
      return false;
    }
    
    DEBUG_printf("SENDING BYTES BELOW: \n");
    DUMP_BYTES(data, size);
    state->waiting_for_ack = !forward;
    
    int count = 0;
    while(state->waiting_for_ack)
    {
        sleep_ms(2);
        count++;
        if(count == 1000) {
            state->waiting_for_ack = false;
            DEBUG_printf("Timeout\n");
            return false;
        }
            
    }
    if (state->got_nak) {
        DEBUG_printf("Got NAK, returning false\n");
        return false;
    }
    return true;
}

bool STANode::handle_incoming_data(unsigned char* buffer, struct pbuf *p) {
    uint32_t source_id = 0;
    bool ACK_flag = false;
    bool NAK_flag = false;
    bool self_reply = false;
    state->got_nak = false;
    
    TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(buffer));
    if (!msg) {
       DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
        ACK_flag = false;
    } else {

        uint8_t msg_id = buffer[1];
        uint16_t len = *reinterpret_cast<uint16_t*>(buffer +2);
        DEBUG_printf("DUMPING STATE BUFFER BELOW:\n");
        DUMP_BYTES(state->buffer, 200);
        DEBUG_printf("DUMPING BYTES RECV BELOW:\n");
        DUMP_BYTES(buffer, 200);
        //dump_bytes(buffer, 100);
        switch (msg_id) {
            case 0x00: {
                TCP_INIT_MESSAGE* initMsg = static_cast<TCP_INIT_MESSAGE*>(msg);
                //does stuff
               DEBUG_printf("Received initialization message from node %u", initMsg->msg.source);
                source_id =  initMsg->msg.source;

                break;
            }
            case 0x01: {
                TCP_DATA_MSG* dataMsg = static_cast<TCP_DATA_MSG*>(msg);
                DEBUG_printf("Got data message\n");
                if (dataMsg->msg.dest == get_NodeID()) {
                    source_id =  dataMsg->msg.source;
                    rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    DEBUG_printf("DBG: Received message from node %08x: %s", dataMsg->msg.source, (char*)dataMsg->msg.msg);
                    ACK_flag = true;
                    break;
                } else {
                    // TODO pass msg to AP
                }
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

                //puts("Update packet recieved");

                //updMsg.

                //does stuff
                break;
            }
            case 0x04: {
                DEBUG_printf("Got ACK\n");
                TCP_ACK_MESSAGE* ackMsg = static_cast<TCP_ACK_MESSAGE*>(msg);

                source_id = ackMsg->msg.source;

               DEBUG_printf("Ack is from %08x and for %08x\n", ackMsg->msg.source, ackMsg->msg.dest);
                if (ackMsg->msg.dest == get_NodeID()) {
                    self_reply = true;
                    DEBUG_printf("ACK is for me\n");
                    state->waiting_for_ack = false;
                }
                //does stuff
                break;
            }
            case 0x05: {
                TCP_NAK_MESSAGE* nakMsg = static_cast<TCP_NAK_MESSAGE*>(msg);
                //does stuff
                puts("Warning! Message was not received! (Got a NAK instead of ACK)");
                DEBUG_printf("Got NAK\n");
                self_reply = true;
                state->waiting_for_ack = false;
                state->got_nak = true;
                break;
            }
            default:
               DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
                ACK_flag = false;
                // SEND NAK message
                //TCP_NAK_MESSAGE nakMsg(node->get_NodeID(), msg_id ? msg_id : 0, 0);
                break;
        }
    }

    if (ACK_flag && !self_reply){
        DEBUG_printf("Sending ack to %08x\n", source_id);
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), source_id, p->tot_len);
        send_tcp_data(ackMsg.get_msg(), ackMsg.get_len(), true);
        //state->waiting_for_ack = true;
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from sender and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), 0, p->tot_len);
        send_tcp_data(nakMsg.get_msg(), nakMsg.get_len(), true);
        delete msg;
        return false;
    }

    delete msg;
    return true;
}

err_t STANode::send_data(uint32_t send_id, ssize_t len, uint8_t *buf) {
    TCP_DATA_MSG msg(get_NodeID(), send_id);
    msg.add_message(buf, len);
    if (!send_tcp_data_blocking(msg.get_msg(), msg.get_len(), false))
        return -1;
    return 0;
}