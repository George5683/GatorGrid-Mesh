#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <map>
#include "pico/cyw43_arch.h"


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
    void set_NodeID(int ID);
    int get_NodeID();

    // Generic function for generating and assiging random IDs
    void generate_id();
    void seed_rand();
};

class APNode : public MeshNode{   
public:
    TCP_SERVER_T* state;
    bool running;
    char ap_name[32];
    uint8_t **buffers;
    ssize_t num_buffers;
    ssize_t number_of_filled_buffers;
    const char* password;

    APNode();
    ~APNode();

    // Initialize hardware and allocate resources
    bool init_ap_mode();
    
    // Start the AP mode and servers
    bool start_ap_mode();

    // Stop the AP mode
    void stop_ap_mode();
    
    // Poll function to handle network events
    void poll(unsigned int timeout_ms = 1000);

    // Getters/setters for AP configuration
    void set_ap_credentials(char *name, const char* pwd);
    
    int get_node_id();
    void set_node_id(int ID);

    bool digest_recv_buffer(uint8_t *buf);
    ssize_t recv_buffer_queue_len();

    // REACH MILESTONES

    void server_test();
};

class STANode : public MeshNode{
public:
    TCP_CLIENT_T* state;
    std::map<uint32_t, cyw43_ev_scan_result_t*> known_nodes;

    STANode();
    ~STANode();

    bool init_sta_mode();
    bool start_sta_mode();
    bool scan_for_nodes();
    bool connect_to_AP(char* SSID, char* password, int ms_timeout);
    void client_test();
    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
    void init_tcp();
};

#endif // MESH_NODE_HPP
