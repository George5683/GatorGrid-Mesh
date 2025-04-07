#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <set>
#include <string>
#include <map>
#include <vector>
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
    void set_NodeID(uint32_t ID);
    uint32_t get_NodeID();
    void seed_rand();
};

class APNode : public MeshNode{   
public:
    TCP_SERVER_T* state;
    bool running;
    char ap_name[32];
    const char* password;

    std::vector<void*> connections;

    // map for results/results_flag from clients
    std::map<int, std::string> client_results;
    std::map<int, bool> client_results_flag;

    // CONSTRUCTOR/DECONSTRUCTOR
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

    void server_test();

    bool send_tcp_data(uint8_t* data, uint32_t size);

};

class STANode : public MeshNode{
public:

    std::map<int, cyw43_ev_scan_result_t*> known_nodes;
    TCP_CLIENT_T* state;

    std::string AP_CONNECTED_IP;
  
    STANode();
    ~STANode();

    bool init_sta_mode();
    bool start_sta_mode();
    bool scan_for_nodes();
    bool connect_to_node(uint32_t id);
    bool is_connected();
    bool tcp_init();

    bool send_tcp_data(uint8_t* data, uint32_t size);
    bool send_string_data(const char* data_string);

    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
};

#endif // MESH_NODE_HPP
