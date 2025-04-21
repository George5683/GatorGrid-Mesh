#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <set>
#include <string>
#include <map>
#include <vector>
#include "pico/cyw43_arch.h"

#include "../SPI/SPI.hpp"
#include "../RingBuffer/RingBuffer.hpp"
#include "Messages.hpp"
#include "../ChildrenTree/ChildrenTree.hpp"

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

// Forward declarations
struct TCP_SERVER_T_;
typedef struct TCP_SERVER_T_ TCP_SERVER_T;

struct TCP_CLIENT_T_;
typedef struct TCP_CLIENT_T_ TCP_CLIENT_T;

class MeshNode {
private:
    uint32_t NodeID;
public:
    MeshNode();
    virtual ~MeshNode();

    // Base class functions for get/set NodeID
    void set_NodeID(uint32_t ID);
    uint32_t get_NodeID();
    void seed_rand();
};

class APNode : public MeshNode{   
public:
    TCP_SERVER_T* state;
    bool running;
    char ap_name[32];
    const char* password = "password";
    
    RingBuffer rb;
    ChildrenTree tree;

    SPI Master_Pico;

    std::vector<void*> connections;

    // map for results/results_flag from clients
    std::map<int, std::string> client_results;
    std::map<int, bool> client_results_flag;

    std::map<uint32_t, tcp_pcb*> client_tpcbs;

    //lp9 CONSTRUCTOR/DECONSTRUCTOR
    APNode();
    APNode(uint32_t id);
    ~APNode();

    // Initialize hardware and allocate resources
    bool init_ap_mode();
    
    // Start the AP mode and servers
    bool start_ap_mode();

    // Stop the AP mode
    void stop_ap_mode();
    
    // Poll function to handle network events
    void poll(unsigned int timeout_ms = 1000);

    // Get the data from the incoming client
    char* get_client_data(int ID);
    // Check for incoming data from a client
    bool has_client_data(int ID);
    // Check data incoming from all connected clients
    bool check_all_client_data();

    // Getters/setters for AP configuration
    void set_ap_credentials(char name[32], const char* pwd);
    
    int get_node_id();
    void set_node_id(int ID);

    void server_start();
    bool server_running();

    int number_of_messages();

    bool send_tcp_data(uint32_t id, tcp_pcb *client_pcb, uint8_t* data, uint32_t size);

    struct data digest_data();

    bool handle_incoming_data(unsigned char* buffer, tcp_pcb* tpcb, struct pbuf *p);

};

class STANode : public MeshNode{
public:

    std::map<int, cyw43_ev_scan_result_t*> known_nodes;
    uint32_t parent;
    TCP_CLIENT_T* state;

    std::string AP_CONNECTED_IP;

    SPI Slave_Pico;


    STANode();
    ~STANode();

    bool init_sta_mode();
    bool start_sta_mode();
    bool scan_for_nodes();
    bool connect_to_node(uint32_t id);
    bool is_connected();
    bool tcp_init();

    err_t send_data(uint32_t send_id, ssize_t len, uint8_t *buf);

    bool send_tcp_data(uint8_t* data, uint32_t size, bool forward);
    bool send_tcp_data_blocking(uint8_t* data, uint32_t size, bool forward);


    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
    bool handle_incoming_data(unsigned char* buffer, struct pbuf *p);
};

#endif // MESH_NODE_HPP
