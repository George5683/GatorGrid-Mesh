#include "MeshNode.hpp"
#include "Messages.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "../SPI/SPI.hpp"
#include "../ChildrenTree/ChildrenTree.hpp"

#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"

#define TCP_PORT 4242

#define BUF_SIZE 2048
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#define DEBUG 0

#if DEBUG 
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#if 1
 static void dump_bytes(const uint8_t *bptr, uint32_t len) {
     unsigned int i = 0;
 
    DEBUG_printf("dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
            DEBUG_printf("\n");
         } else if ((i & 0x07) == 0) {
            DEBUG_printf(" ");
         }
        DEBUG_printf("%02x ", bptr[i++]);
     }
    DEBUG_printf("\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif

 #define TRY(x)            \
 do                        \
 {                         \
     err_t st;             \
     st = (x);             \
     if (st != 0)          \
         return st;        \
 } while (0)

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
    int sent_len;
    int recv_len;
    int run_count;
    ip_addr_t gw;
    void* ap_node;
    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
} TCP_SERVER_T;

struct ClientConnection {
    struct tcp_pcb *pcb;
    bool id_recved = false;
    uint32_t id = 0;
};
 
std::vector<ClientConnection> clients;
std::map<tcp_pcb *, ClientConnection> clients_map;


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

err_t tcp_server_close(void *arg) {
    APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = node->state;
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
    // APNode* node = (APNode*)arg;
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    APNode* node = (APNode*)state->ap_node;

    bool got_full_message = false;

   DEBUG_printf("Recv called\n");
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

        if(clients_map.find(tpcb) != clients_map.end()) {
           DEBUG_printf("PCB FOUND\n");
        } else {
           DEBUG_printf("MAJOR ERROR CLIENT PCB NOT FOUND\n");
            return ERR_OK; // TODO: Change error handling
        }

        node->handle_incoming_data(state->buffer_recv, tpcb, p);

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

    struct ClientConnection client;

    client.pcb = client_pcb;

    clients.push_back({client});

    clients_map.insert({client_pcb, client});

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
APNode::APNode() : state(nullptr), running(false), password("password"), rb(10), tree(get_NodeID()), Master_Pico()  {
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
}

APNode::APNode(uint32_t id) : state(nullptr), running(false), password("password"), rb(10), tree(id), Master_Pico()  {
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

    // initalize SPI
    Master_Pico.Set_Master(true);

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
 \
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
    Master_Pico.SPI_init();

    uint32_t ID = this->get_node_id();
    vector<uint8_t> temp =  { 'A', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f' };
    *(uint32_t*)temp.data() = ID;

    Master_Pico.SPI_send_message(temp);

    puts("Message sent");
    //puts("entering poll test");

    //while(!Master_Pico.SPI_POLL_MESSAGE());

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

bool APNode::send_tcp_data(uint32_t id, tcp_pcb *client_pcb, uint8_t* data, uint32_t size) {

    //uint8_t buffer[size] = {};
    // if (size > BUF_SIZE) { size = BUF_SIZE; }
    //memcpy(buffer, data, size);

    bool flag = false;
    dump_bytes(data, size);

    cyw43_arch_lwip_begin();

    // not entirely sure its supposed to be client_pcb
    
    //err_t err = tcp_write(state->client_pcb, (void*)buffer, size, TCP_WRITE_FLAG_COPY);
    //err_t err2 = tcp_output(state->client_pcb);

    DEBUG_printf("send_tcp_data: Destination ID %08x\n", id);

    err_t err = tcp_write(client_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);
    err_t err2 = tcp_output(client_pcb);

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
   DEBUG_printf("Successfully queued message\n");
    return true;
}


bool APNode::handle_incoming_data(unsigned char* buffer, tcp_pcb* tpcb, struct pbuf *p) {
    bool ACK_flag = true;
    bool NAK_flag = false;
    bool update_flag = false;
    uint8_t error = 0;
    // tcp_init_msg_t *init_msg_str = reinterpret_cast <tcp_init_msg_t *>(state->buffer_recv);
    // clients_map[tpcb].id = init_msg_str->source;
    //DEBUG_printf("ID RECV FROM INIT MESSAGE: %08X\n", clients_map[tpcb].id);
    // uint32_t test = ((APNode*)(state->ap_node))->get_NodeID();
    //DEBUG_printf("CURRENT NODE ID: %08X\n", test);

    puts("HANDLE DUMP");
    dump_bytes(buffer, 100); 

    // TCP_INIT_MESSAGE init_msg(((APNode*)(state->ap_node))->get_NodeID());  
    TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(state->buffer_recv));
    uint8_t msg_id = 0xFF;

    if (!msg) {
       DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
        ACK_flag = false;
    } else {

        // TODO: Handle error checks for messages
        msg_id = buffer[1];

        //DUMP_BYTES(state->buffer_recv, 2048);

        switch (msg_id) {
            case 0x00: {
                TCP_INIT_MESSAGE* initMsg = static_cast<TCP_INIT_MESSAGE*>(msg);
               DEBUG_printf("Received initialization message from node %u", initMsg->msg.source);
                //does stuff

                // Set init node ID
                if(clients_map[tpcb].id_recved == false) {
                    ACK_flag = true;
                    clients_map[tpcb].id_recved = true;
                    clients_map[tpcb].id = initMsg->msg.source;
                }

                tree.add_child(initMsg->msg.source);

                // Insert into map of IDs to TPCB
                client_tpcbs.insert({clients_map[tpcb].id, tpcb});

                update_flag = true;
                
                break;
            }
            case 0x01: {
                TCP_DATA_MSG* dataMsg = static_cast<TCP_DATA_MSG*>(msg);
                // Store messages in a ring buffer
                puts("Got data message");
               DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                if (dataMsg->msg.dest == get_NodeID()) {
                    rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    DEBUG_printf("Successfully inserted into ring buffer\n");
                    ACK_flag = true;
                    break;
                } else {
                    uint32_t dest;
                   DEBUG_printf("Message was not for me, was for dest:%08x\n", dataMsg->msg.dest);
                    // if(!tree.find_path_parent(dataMsg->msg.dest, &dest)) {
                    //     DEBUG_printf("Count not find path to parent\n");
                    //     ACK_flag = false;
                    //     //NAK_flag = true;
                    //     error = 0x01; // TODO make enum for errors (no node in tree)
                    //     TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                    //     nakMsg.set_error(error);
                    //     send_tcp_data(dataMsg->msg.source, client_tpcbs.at(dataMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                    //     break;
                    // }
                    // DEBUG_printf("Found path to parent, starts with %u\n", dest);
                    if (client_tpcbs.find(dataMsg->msg.dest) == client_tpcbs.end()) {
                        ACK_flag = false;
                        //NAK_flag = true;
                        error = 0x02; // TODO make enum for errors (id not in connected clients)
                        TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                        nakMsg.set_error(error);
                        send_tcp_data(dataMsg->msg.source, client_tpcbs.at(dataMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                        break;
                    }
                    ACK_flag = false;
                    DEBUG_printf("Forwarding message to node\n");
                    send_tcp_data(dataMsg->msg.dest, client_tpcbs.at(dataMsg->msg.dest), dataMsg->get_msg(), dataMsg->get_len());
                    break;
                }
                
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
                // Do not need to respond to acks
                if (ackMsg->msg.dest == get_NodeID()) {
                    //rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    ACK_flag = false;
                } else {
                    uint32_t dest;
                   DEBUG_printf("Message was not for me, was for dest:%08x\n", ackMsg->msg.dest);
                    // if(!tree.find_path_parent(ackMsg->msg.dest, &dest)) {
                    //     ACK_flag = false;
                    //     NAK_flag = true;
                    //     error = 0x01; // TODO make enum for errors (no node in tree)
                    //     break;
                    // }
                    if (client_tpcbs.find(ackMsg->msg.dest) == client_tpcbs.end()) {
                        ACK_flag = false;
                        //NAK_flag = true;
                        error = 0x02; // TODO make enum for errors (id not in connected clients)
                        TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                        nakMsg.set_error(error);
                        send_tcp_data(ackMsg->msg.source, client_tpcbs.at(ackMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                        break;
                    }
                    ACK_flag = false;
                    send_tcp_data(ackMsg->msg.dest, client_tpcbs.at(ackMsg->msg.dest), ackMsg->get_msg(), ackMsg->get_len());
                }
                


                break;
            }
            case 0x05: {
                TCP_NAK_MESSAGE* nakMsg = static_cast<TCP_NAK_MESSAGE*>(msg);
                ACK_flag = false;
                //does stuff
                break;
            }
            default:
               DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
                ACK_flag = false;
                break;
        }
    }

    if (update_flag) {
        puts("Update nodes called");
        /*puts("Broadcasting all connected nodes to each other");
        uint32_t ids[4] = {0};
        int j = 0;

        // Because this is a new node, resend the list of connected nodes to everyone
        for(auto i : client_tpcbs) {
            ids[j] = i.first;
            j++;
        }

        TCP_UPDATE_MESSAGE updateMsg(get_NodeID());
        updateMsg.add_children(j, ids);

        for(auto i : client_tpcbs) {
            sleep_ms(20);
            send_tcp_data(i.first, i.second, updateMsg.get_msg(), updateMsg.get_len());
           DEBUG_printf("Sent update message to %08x\n", i.first);
        }*/
    }

    if (ACK_flag){
       DEBUG_printf("Sending ACK message to client %08x\n", clients_map[tpcb].id);
        // Assumption: clients_map[tpcb].id implies that any message worth acking isn't being forwarded and is originating from the intended node
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
        send_tcp_data(clients_map[tpcb].id, tpcb, ackMsg.get_msg(), ackMsg.get_len());
        //send_tcp_data(ackMsg.get_msg(), ackMsg.get_len());
       DEBUG_printf("Sent ACK message to client %08x\n", clients_map[tpcb].id);
        
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from clients_map and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
        nakMsg.set_error(error);
        send_tcp_data(clients_map[tpcb].id, tpcb, nakMsg.get_msg(), nakMsg.get_len());
        //send_tcp_data(nakMsg.get_msg(), nakMsg.get_len());
        delete msg;
        return false;
    } 

    delete msg;
    return true;
}


err_t APNode::send_data(uint32_t send_id, ssize_t len, uint8_t *buf) {
    //TODO: Add tree and look whether it needs to go up or down the tree
    TCP_DATA_MSG msg(get_NodeID(), send_id);
    msg.add_message(buf, len);
    if (client_tpcbs.find(send_id) == client_tpcbs.end()){
        return -2;
    } 
    tcp_pcb *client_pcb = client_tpcbs.at(send_id);
    if (!send_tcp_data(send_id, client_pcb, msg.get_msg(), msg.get_len()))
        return -1;
    return 0;
}

err_t APNode::send_msg(uint8_t* msg) {
    TCP_MESSAGE* tcp_msg = parseMessage(msg);
    size_t len = 0;
    uint32_t target_id = 0;
    tcp_pcb *target = nullptr;

    uint8_t id = msg[1];
    switch (id) {
        case 0x00:  // Init message from STA -> thus new parent has been added
        {           // This message should probably never be received via serial
            TCP_INIT_MESSAGE* initMsg = static_cast<TCP_INIT_MESSAGE*>(tcp_msg);
            parent = initMsg->msg.source;

            return 0;
            /* TODO: This is for STA
                
                // len = initMsg->get_len();
                // target_id = initMsg->msg.source; // ??

                // Will have to send Update Message to parent
                tree.add_any_child(get_node_id(), initMsg->msg.source);
                

                uint32_t children[4];
                uint8_t children_count = 0;
                if (!tree.get_children(get_node_id(), children, children_count))
                {
                    //idk die or something
                    // TODO: failure states
                }

                TCP_UPDATE_MESSAGE updateMsg(get_node_id());
                updateMsg.add_children(children_count, children);

                target_id = 
            */
        }
        case 0x01: /* TCP_DATA_MSG */
        {
            TCP_DATA_MSG* dataMsg = static_cast<TCP_DATA_MSG*>(tcp_msg);
            len = dataMsg->get_len();
            target_id = dataMsg->msg.dest;

            break;
        }
        case 0x02:
            break;
        case 0x03:
            break;
        default:
        {
            return -1;
        }

        // TODO: target = find(target_id);
        uint32_t parent;
        tree.find_path_parent(target_id, &parent);
        target = client_tpcbs.at(target_id);
        send_tcp_data(target_id, target, msg, len);
        
    }
    return 0;
}

err_t APNode::handle_serial_message(uint8_t *msg) {
    uint8_t id = serialMessageType(msg);

    // TODO: Finish switch-case
    switch (id) {
        case 0x00: /* serial_node_add_msg */
        {
            // If AP receives node_add, a parent has been added, maybe?
            SERIAL_NODE_ADD_MESSAGE serial_msg;
            serial_msg.set_msg(msg);

            // TODO: figure which to set parent = to
            break; 
        }
        case 0x01: /* serial_node_remove_msg */
            // If AP receives node_remove, a parent has been removed
            break;
        case 0x02: /* Data message */
        {
            // SERIAL_DATA_MESSAGE serial_msg;
            // serial_msg.set_msg(msg);

            send_msg(msg);
            
            break;
        }
        case 0x03: /* serial_fatal_error_msg */
        {
            break;
        }
        default:
        {
            return 0;
        }
    }

    return 0;
}





/*
(send from STA-A to Node C)
[STA-A -- AP-A] <----->  [STA-B <-----> AP-B]
                                        \
                                        \
                                    [STA-C <-----> AP-C]

STA-A -> send serial to AP-A                                             (redundant?)
AP-A recieves serial, sees its DATA passes it, (handle_serial -> send_msg -> send_data -> send_tcp_data) A->C

                                            (redundant?)
STA-B sees its DATA -> sees its for C (looks in tree, finds its a parent) -> sends serial to AP-B
AP-B recieves serial, sees its DATA passes it, (handle_serial -> send_msg -> send_data -> send_tcp_data) A->C
*/