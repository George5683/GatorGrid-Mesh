#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <set>
#include <map>
#include "pico/cyw43_arch.h"

// Forward declarations
struct TCP_SERVER_T_;
typedef struct TCP_SERVER_T_ TCP_SERVER_T;

struct TCP_CLIENT_T_;
typedef struct TCP_CLIENT_T_ TCP_CLIENT_T;

class MeshNode {
private:
    int NodeID;
public:
    MeshNode();
    virtual ~MeshNode();

    // Base class functions for get/set NodeID
    void set_NodeID(int ID);
    int get_NodeID();
};

class APNode : public MeshNode{   
public:
    TCP_SERVER_T* state;
    bool running;
    char ap_name[32];
    const char* password;
    // Flag to track if the webpage is enabled
    bool webpage_enabled;

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
    void set_ap_credentials(char name[32], const char* pwd);
    
    int get_node_id();
    void set_node_id(int ID);

    // REACH MILESTONES

    // function to enable a webpage on the access point
    void enable_webpage();

    // function to disable a webpage on the access point
    void disable_webpage();

};

class STANode : public MeshNode{
public:

    std::map<int, cyw43_ev_scan_result_t*> known_nodes;
    TCP_CLIENT_T* state;
  

    STANode();
    ~STANode();

    bool init_sta_mode();
    bool start_sta_mode();
    bool scan_for_nodes();
    bool connect_to_node(uint32_t id);

    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
};

#endif // MESH_NODE_HPP
