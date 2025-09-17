#ifndef MESH_NODE_HPP
#define MESH_NODE_HPP

#include <cstdint>
#include <lwip/err.h>
#include <set>
#include <string>
#include <map>
#include <vector>
#include "pico/cyw43_arch.h"

#include "../SPI/SPI.hpp"
#include "../RingBuffer/RingBuffer.hpp"
#include "../SPI/SerialMessages.hpp"
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
    uint32_t parent;
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

    /**
     * @brief Initialize hardware and allocate resources
     * 
     * @return true     - successful
     * @return false    - failure
     */
    bool init_ap_mode();
    
    /**
     * @brief Start the AP mode and servers
     * 
     * @return true     - successful
     * @return false    - failure
     */
    bool start_ap_mode();

    /**
     * @brief Stop the AP mode
     * 
     */
    void stop_ap_mode();
    
    /**
     * @brief Poll function to handle network events
     * 
     * @param timeout_ms 
     */
    void poll(unsigned int timeout_ms = 1000);

    /**
     * @brief Get the data from the incoming client
     * 
     * @param ID 
     * @return char* (no implementation??)
     */
    char* get_client_data(int ID);

    /**
     * @brief Check for incoming data from a client
     * 
     * @param ID 
     * @return true 
     * @return false 
     */
    bool has_client_data(int ID);
    // Check data incoming from all connected clients
    bool check_all_client_data();

    // Getters/setters for AP configuration
    void set_ap_credentials(char name[32], const char* pwd);
    
    /**
     * @brief Get the node id object of APNode
     * 
     * @return int 
     */
    int get_node_id();

    /**
     * @brief Set the node id object
     * 
     * @param ID 
     */
    void set_node_id(int ID);

    /**
     * @brief Start server
     * 
     */
    void server_start();

    /**
     * @brief Returns if server is currently running
     * 
     * @return true 
     * @return false 
     */
    bool server_running();

    /**
     * @brief Number of messages in ring buffer
     * 
     * @return int 
     */
    int number_of_messages();

    /**
     * @brief Sends Data packet to a given node
     * 
     * @param send_id   - Node receiving the data packet
     * @param len       - length of buffer
     * @param buf       - formatted data packet buffer
     * @return err_t 
     */
    err_t send_data(uint32_t send_id, ssize_t len, uint8_t *buf);

    /**
     * @brief Writes data to correct tcp buffer
     * 
     * @param id            - Id of reciever (not used lol)
     * @param client_pcb    - tcp_pcb* of target
     * @param data          - message buffer
     * @param size          - length of message buffer
     * @return true 
     * @return false 
     */
    bool send_tcp_data(uint32_t id, tcp_pcb *client_pcb, uint8_t* data, uint32_t size);

    struct data digest_data();

    /**
     * @brief Handles incoming data
     *
     * @details Manages operations for dealing with different types of messages
     *          being received. 
     * 
     * @param buffer    - dara buffer
     * @param tpcb      - tcp_pcb* of source
     * @param p         - description of buffer
     * @return true 
     * @return false 
     */
    bool handle_incoming_data(unsigned char* buffer, tcp_pcb* tpcb, struct pbuf *p);

    /**
     * @brief Sends formatted Messages(Under construction)
     * 
     * @param msg   - formatted message buffer
     * @return err_t 
     */
    err_t send_msg(uint8_t* msg);

    /**
     * @brief Handles incoming serial data
     * 
     * @param msg   - formatted message buffer 
     * @return err_t 
     */
    err_t handle_serial_message(uint8_t* msg);
};

class STANode : public MeshNode{
public:

    std::map<int, cyw43_ev_scan_result_t*> known_nodes;
    uint32_t parent;
    TCP_CLIENT_T* state;

    std::string AP_CONNECTED_IP;

    RingBuffer rb;

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

    struct data digest_data();
    int number_of_messages();

    static int scan_result(void* env, const cyw43_ev_scan_result_t* result);
    bool handle_incoming_data(unsigned char* buffer, struct pbuf *p);

    err_t send_msg(uint8_t *msg);
    err_t handle_serial_message(uint8_t* msg);
};

#endif // MESH_NODE_HPP
