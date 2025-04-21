#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include <cstdint>
#include <cstring>

class TCP_MESSAGE {
protected:
    uint8_t priority;
public:
    TCP_MESSAGE(uint8_t priority) : priority(priority) {}
    ~TCP_MESSAGE() { }
    virtual uint8_t* get_msg() = 0;
    virtual uint16_t get_len() = 0;
    virtual void set_msg(void* msg) = 0;

    typedef struct __attribute__((__packed__)) tcp_init_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
    }tcp_init_msg;

    typedef struct __attribute__((__packed__)) tcp_update_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
        uint8_t child_count;
        uint32_t children_IDs[4]; //max 4
    }tcp_update_msg;

    typedef struct __attribute__((__packed__)) tcp_data_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
        uint32_t dest;
        uint16_t msg_len;
        uint8_t msg[2034]; //max message len left for 2048 bytes
    }tcp_data_msg;

    typedef struct __attribute__((__packed__)) tcp_disconnect_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
        uint8_t cause; //0 if purposeful, 1-127 (0x7F) personal errors, 128-255 (0x80-0xFF)
        // for other nodes losing child node
        uint32_t lost_node; // ignored if 0-127 cause
        uint8_t child_count;
        uint32_t children_IDs[4]; //max 4
    }tcp_disconnect_msg;

    typedef struct __attribute__((__packed__)) tcp_acknowledge_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
        uint32_t dest;
        uint16_t bytes_received; // TODO:: add error check val in future
    }tcp_acknowledge_msg;

    typedef struct __attribute__((__packed__)) tcp_failed_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
        uint32_t source;
        uint32_t dest;
        uint16_t bytes_received; // TODO:: add error check val in future
        uint8_t error;
    }tcp_failed_msg;

    typedef struct __attribute__((__packed__)) tcp_force_update_msg_t{
        uint8_t priority;
        uint8_t msg_id;
        uint16_t len;
    }tcp_force_update_msg; // its 10pm do you know where your children are??
};



class TCP_INIT_MESSAGE : public TCP_MESSAGE {
public:
    tcp_init_msg msg = {0};
public:
    TCP_INIT_MESSAGE(uint32_t id) : TCP_MESSAGE(0xFF) { 
        msg.priority = priority;
        msg.msg_id = 0x00;
        msg.len = 8;
        msg.source = id;
    }
    

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
    uint16_t get_len() override {
        return msg.len;
    }

    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_init_msg*>(msg));
    }
};

class TCP_DATA_MSG : public TCP_MESSAGE {
public:
    tcp_data_msg msg = {0};
public:
    TCP_DATA_MSG(uint32_t src_id, uint32_t dest_id) : TCP_MESSAGE(0x7F) {
        msg.priority = priority;
        msg.msg_id = 0x01;
        msg.len = 14;
        msg.source = src_id;
        msg.dest = dest_id;
    }

    void add_message(uint8_t* msg_i, uint8_t msg_len) {
        memcpy(msg.msg, msg_i, msg_len > 2034 ? 2034 : msg_len);
        msg.msg_len = msg_len;
        msg.len += msg_len > 2034 ? 2034 : msg_len;
    }

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
    uint16_t get_len() override {
        return msg.len;
    }
    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_data_msg*>(msg));
    }
};


class TCP_DISCONNECT_MSG : public TCP_MESSAGE {
public:
    tcp_disconnect_msg msg = {0};
public:
    TCP_DISCONNECT_MSG(uint32_t src_id) : TCP_MESSAGE(0xFF) {
        msg.priority = priority;
        msg.msg_id = 0x02;
        msg.source = src_id;
        msg.len = 30;
    }

    void lost_node(uint32_t lost_node, uint8_t cause) {
        msg.lost_node = lost_node;
        msg.cause = cause;
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
    uint16_t get_len() override {
        return msg.len;
    }
    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_disconnect_msg*>(msg));
    }
};

class TCP_UPDATE_MESSAGE : public TCP_MESSAGE {
public:
    tcp_update_msg msg = {0};
public:
    TCP_UPDATE_MESSAGE(uint32_t id) : TCP_MESSAGE(0xFF) { 
        msg.priority = priority;
        msg.msg_id = 0x03;
        msg.len = 25;
        msg.source = id;
        msg.child_count = 0;
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
    uint16_t get_len() override {
        return msg.len;
    }
    uint8_t get_child_count() {
        return msg.child_count;
    }
    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_update_msg*>(msg));
    }
};
    
class TCP_ACK_MESSAGE : public TCP_MESSAGE {
public:
    tcp_acknowledge_msg msg = {0};
public:
    TCP_ACK_MESSAGE(uint32_t src_id, uint32_t dest_id, uint16_t bytes_received) : TCP_MESSAGE(0xFF) { 
        msg.priority = priority;
        msg.msg_id = 0x04;
        msg.len = 14;
        msg.source = src_id;
        msg.dest = dest_id;
        msg.bytes_received = bytes_received;
    }

    

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
    uint16_t get_len() override {
        return msg.len;
    }
    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_acknowledge_msg*>(msg));
    }
};

class TCP_NAK_MESSAGE : public TCP_MESSAGE {
public:
    tcp_failed_msg msg = {0};
public:
    TCP_NAK_MESSAGE(uint32_t src_id, uint32_t dest_id, uint16_t bytes_received) : TCP_MESSAGE(0xFF) { 
        msg.priority = priority;
        msg.msg_id = 0x05;
        msg.len = 15;
        msg.source = src_id;
        msg.dest = dest_id;
        msg.bytes_received = bytes_received;
    }

    

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
    uint16_t get_len() override {
        return msg.len;
    }
    void set_error(uint8_t err) {
        msg.error = err;
    }
    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<tcp_failed_msg*>(msg));
    }
};



/**
 * @brief Takes in some tcp message buffer and returns a pointer to a new TCP_MESSAGE
 * 
 * @param data buffer recieved over tcp
 * @return TCP_MESSAGE*  !!USER MUST DELETE!!
 */
TCP_MESSAGE* parseMessage(uint8_t* data);

#endif // MESSAGES_HPP