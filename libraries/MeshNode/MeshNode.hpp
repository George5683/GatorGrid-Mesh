#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <set>
#include <map>
#include "pico/cyw43_arch.h"

#define BUF_SIZE 2048

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
    void seed_rand();
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

class TCP_MESSAGE {
protected:
    uint8_t priority;
public:
    TCP_MESSAGE(uint8_t priority) : priority(priority) {}
    ~TCP_MESSAGE() { }
    virtual uint8_t* get_msg() = 0;
};

typedef struct __attribute__((__packed__)) {
    uint8_t priority;
    uint8_t msg_id;
    uint32_t source;

    uint8_t child_count;
    uint32_t children_IDs[4]; //max 4
    uint8_t empty[233]; //empty 233 bytes for 2048 len msg
}tcp_init_msg_t;

class TCP_INIT_MESSAGE : public TCP_MESSAGE {
    tcp_init_msg_t msg = {0};
public:
    TCP_INIT_MESSAGE(uint32_t id) : TCP_MESSAGE(0xFF) { 
        msg.priority = priority;
        msg.msg_id = 0x00;
        msg.source = id;
    }
    void add_children(uint8_t children_count, uint32_t* children) {
        msg.child_count = children_count;
        for (int i = 0; i < children_count; i++) {
            msg.children_IDs[i] = children[i];
        }
    }

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
};

typedef struct __attribute__((__packed__)) {
    uint8_t priority;
    uint8_t msg_id;
    uint32_t source;
    uint32_t dest;
    uint8_t msg_len;
    uint8_t msg[245]; //max message len left for 2048 bytes
}tcp_data_msg_t;

class TCP_DATA_MSG : public TCP_MESSAGE {
    tcp_data_msg_t msg = {0};
public:
    TCP_DATA_MSG(uint32_t src_id, uint32_t dest_id) : TCP_MESSAGE(0x7F) {
        msg.priority = priority;
        msg.msg_id = 0x01;
        msg.source = src_id;
        msg.dest = dest_id;
    }

    void add_message(uint8_t msg_len, uint8_t* msg_i) {
        memcpy(msg.msg, msg_i, msg_len > 244 ? 244 : msg_len);
    }

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
};





class STANode : public MeshNode{
public:

    std::map<int, cyw43_ev_scan_result_t*> known_nodes;
    uint32_t upstream_node = ~0;
    int recieved_bytes = 0;
    TCP_CLIENT_T* state;
  

    STANode();
    ~STANode();

    bool init_sta_mode();
    bool start_sta_mode();
    bool scan_for_nodes();
    bool connect_to_node(uint32_t id);
    bool is_connected();
    bool tcp_init();

    bool send_tcp_data(uint8_t* data, uint32_t size);

    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
};

#endif // MESH_NODE_HPP
