#ifndef SERIAL_MESSAGE_HPP
#define SERIAL_MESSAGE_HPP

#include <cstdint>
#include <cstring>

class SERIAL_MESSAGE {
protected:
    uint8_t priority;
public:
    SERIAL_MESSAGE() { }
    ~SERIAL_MESSAGE() { }
    virtual uint8_t* get_msg() = 0;
    virtual uint16_t get_len() = 0;
    virtual void set_msg(void* msg) = 0;

    typedef struct __attribute__((__packed__)) serial_node_add_msg_t{
        uint8_t msg_id;
        uint16_t len;
        uint32_t parent;
        uint32_t child;
    }serial_node_add_msg;

    typedef struct __attribute__((__packed__)) serial_node_remove_msg_t{
        uint8_t msg_id;
        uint16_t len;
        uint32_t node;
    }serial_node_remove_msg;

    typedef struct __attribute__((__packed__)) serial_data_msg_t{
        uint8_t msg_id;
        uint16_t len;
        uint16_t msg_len;
        uint8_t msg[512-5]; //max message len left for 512 bytes
    }serial_data_msg;

    typedef struct __attribute__((__packed__)) serial_fatal_error_msg_t{
        uint8_t msg_id;
        uint16_t len;
        uint32_t error;
    }serial_fatal_error_msg;
};


class SERIAL_NODE_ADD_MESSAGE : public SERIAL_MESSAGE {
public:
    serial_node_add_msg_t msg = {0};
public:
    SERIAL_NODE_ADD_MESSAGE(uint32_t id, uint32_t parent, uint32_t child) : SERIAL_MESSAGE() { 
        msg.msg_id = 0x00;
        msg.len = 7;
        msg.child = child;
        msg.parent = parent;
    }

    SERIAL_NODE_ADD_MESSAGE() : SERIAL_MESSAGE() {}
    
    uint16_t get_len() override {
        return msg.len;
    }

    uint32_t get_parent() {
        return msg.parent;
    }

    uint32_t get_child() {
        return msg.child;
    }

    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<serial_node_add_msg_t*>(msg));
    }

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
};

class SERIAL_NODE_REMOVE_MESSAGE : public SERIAL_MESSAGE {
public:
    serial_node_remove_msg_t msg = {0};
public:
    SERIAL_NODE_REMOVE_MESSAGE(uint32_t node) : SERIAL_MESSAGE() { 
        msg.msg_id = 0x01;
        msg.len = 11;
        msg.node = node;
    }

    uint32_t get_node() {
        return msg.node;
    }
    
    uint16_t get_len() override {
        return msg.len;
    }

    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }
};

class SERIAL_DATA_MESSAGE : public SERIAL_MESSAGE {
public:
    serial_data_msg_t msg = {0};
public:
    SERIAL_DATA_MESSAGE() : SERIAL_MESSAGE() {
        msg.msg_id = 0x02;
        msg.msg_len = 0; // size of network buffer packet
    }

    void add_message(uint8_t* msg_i, uint8_t msg_len) {
        memcpy(msg.msg, msg_i, msg_len > 512 - 5 ? 512 - 5 : msg_len);
        msg.msg_len = msg_len > 512 - 5 ? 512 - 5 : msg_len;
        msg.len = msg.msg_len + 5;
    }

    uint8_t* get_data() {
        return reinterpret_cast<uint8_t*>(msg.msg);
    }

    // Get the whole serial message
    uint8_t* get_msg() override {
        return reinterpret_cast<uint8_t*>(&msg);
    }

    uint16_t get_msg_len()  {
        return msg.msg_len;
    }

    uint16_t get_len() override {
        return msg.len;
    }

    void set_msg(void* msg) override {
        this->msg = *(reinterpret_cast<serial_data_msg_t*>(msg));
    }
};

class SERIAL_FATAL_ERROR_MESSAGE : public SERIAL_MESSAGE {
public:
    serial_fatal_error_msg_t msg = {0};
public:
    SERIAL_FATAL_ERROR_MESSAGE(uint32_t error) : SERIAL_MESSAGE() { 
        msg.msg_id = 0x03;
        msg.len = 7;
        msg.error = error;
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
        this->msg = *(reinterpret_cast<serial_fatal_error_msg_t*>(msg));
    }
};



/**
 * @brief Takes in some serial message buffer and returns a pointer to a new SERIAL_MESSAGE
 * 
 * @param data buffer recieved over serial
 * @return SERIAL_MESSAGE*  !!USER MUST DELETE!!
 */
SERIAL_MESSAGE* parseSerialMessage(uint8_t* data);

/**
 * @brief Returns ID of serial message inserted
 * 
 * @param data 
 * @return uint8_t 
 */
uint8_t serialMessageType(uint8_t* data);

#endif // SERIAL_MESSAGE_HPP
