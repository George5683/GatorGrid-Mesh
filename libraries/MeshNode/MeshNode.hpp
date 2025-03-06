#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>

// Forward declarations
struct TCP_SERVER_T_;
typedef struct TCP_SERVER_T_ TCP_SERVER_T;

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
        const char* ap_name;
        const char* password;

        APNode();
        ~APNode();

        // Initialize hardware and allocate resources
        bool init_ap_mode();
        
        // Start the AP mode and servers
        bool start_ap_mode();
        
        // Poll function to handle network events
        void poll(unsigned int timeout_ms = 1000);
        
        // Stop the AP mode
        void stop_ap_mode();
        
        // Getters/setters for AP configuration
        void set_ap_credentials(const char* name, const char* pwd);
        
        int get_node_id();
        void set_node_id(int ID);
};

#endif // MESH_NODE_HPP