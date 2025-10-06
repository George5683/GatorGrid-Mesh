#include "MeshNode.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "Messages.hpp"
#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "display.hpp"

#define DATA_MSG 0x01

 bool APNode::send_tcp_data(uint32_t id, tcp_pcb *client_pcb, uint8_t* data, uint32_t size) {

    //uint8_t buffer[size] = {};
    // if (size > BUF_SIZE) { size = BUF_SIZE; }
    //memcpy(buffer, data, size);

    bool flag = false;
    DUMP_BYTES(data, size);

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
    bool send_to_sta = false;

    uint32_t msg_source = 0;

    uint8_t error = 0;
    // tcp_init_msg_t *init_msg_str = reinterpret_cast <tcp_init_msg_t *>(state->buffer_recv);
    // clients_map[tpcb].id = init_msg_str->source;
    //DEBUG_printf("ID RECV FROM INIT MESSAGE: %08X\n", clients_map[tpcb].id);
    // uint32_t test = ((APNode*)(state->ap_node))->get_NodeID();
    //DEBUG_printf("CURRENT NODE ID: %08X\n", test);

    DEBUG_printf("HANDLE DUMP\n");
    DUMP_BYTES(buffer, p->tot_len); 

    // TCP_INIT_MESSAGE init_msg(((APNode*)(state->ap_node))->get_NodeID());  
    //TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(state->buffer_recv));
    uint8_t msg_id = 0xFF;

    // if (!msg) {
    //    DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
    //     ACK_flag = false;
    // } else {

    // TODO: Handle error checks for messages
    msg_id = buffer[1];
    uint16_t len = (buffer[3] << 8) | buffer[2];

        //DUMP_BYTES(state->buffer_recv, 2048);

    switch (msg_id) {
        case 0x00: {
            TCP_INIT_MESSAGE initMsg;// = reinterpret_cast<TCP_INIT_MESSAGE*>(buffer);
            initMsg.set_msg(buffer);

            msg_source = initMsg.msg.source;

            DEBUG_printf("Received initialization message from node %u\n", msg_source);
            DUMP_BYTES(initMsg.get_msg(), len);
            //does stuff

            // Set init node ID
            if(clients_map[tpcb].id_recved == false) {
                ACK_flag = true;
                clients_map[tpcb].id_recved = true;
                clients_map[tpcb].id = msg_source;
            }

            
            DEBUG_printf("\nMy ID = %u\n", get_NodeID());
            DEBUG_printf(tree.add_any_child(get_NodeID() ,msg_source) ? "Child Added Successfully" : "Child Failed to be Added");

            // Insert into map of IDs to TPCB
            client_tpcbs.insert({clients_map[tpcb].id, tpcb});

            SERIAL_NODE_ADD_MESSAGE addmsg(get_NodeID(), msg_source);
            uart.sendMessage((char*)addmsg.get_msg());

            update_flag = true;
            
            break;
        }
        case 0x01: {
            
            TCP_DATA_MSG dataMsg;// = reinterpret_cast<TCP_DATA_MSG*>(state->buffer_recv);
            dataMsg.set_msg(buffer);

            msg_source = dataMsg.msg.source;

            // Store messages in a ring buffer
            DEBUG_printf("Got data message\n");
            DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg.msg.msg_len, msg_source, dataMsg.msg.dest);
            if (dataMsg.msg.dest == get_NodeID()) {
                //rb.insert(dataMsg.msg.msg, dataMsg.msg.msg_len, dataMsg.msg.source, dataMsg.msg.dest);
                // uart.sendMessage((char*) buffer);
                // DEBUG_printf("Successfully inserted into ring buffer\n");
                send_to_sta = true;
                ACK_flag = true;
                break;
            } else {
                uint32_t dest = dataMsg.msg.dest;
                uint32_t path_parent = 0;
                DEBUG_printf("Message was not for me, was for dest:%08x\n", dataMsg.msg.dest);

                if(tree.find_path_parent(dataMsg.msg.dest, &path_parent)) {
                    send_msg(buffer);
                } else {
                    send_to_sta = true;
                }
                ACK_flag = false;
                // if (client_tpcbs.find(dataMsg.msg.dest) != client_tpcbs.end()) {
                //     ACK_flag = false;
                //     DEBUG_printf("Forwarding message to directly connected node\n");
                //     send_tcp_data(dataMsg.msg.dest, client_tpcbs.at(dataMsg.msg.dest), dataMsg.get_msg(), dataMsg.get_len());
                //     break;

                //     // Check the node tree and see if it is a child
                // } else if (is_root == false) {

                //     DEBUG_printf("Node not found in our connection, forwarding to STA to forward up");
                    
                //     // Construct serial message for STA
                //     ACK_flag = false;
                //     NAK_flag = false;

                //     send_to_sta = true;
                        

                //     // If the node is not in our tree and we are not root kill the message
                // } else if (is_root == true) {
                //     ACK_flag = false;
                //     NAK_flag = true;
                //     // error = 0x02; // TODO make enum for errors (id not in connected clients)
                //     // TCP_NAK_MESSAGE nakMsg(get_NodeID(), msg_source, p->tot_len);
                //     // nakMsg.set_error(error);
                //     // send_tcp_data(dataMsg.msg.source, client_tpcbs.at(dataMsg.msg.source), nakMsg.get_msg(), nakMsg.get_len());
                //     break;
                // }
            }
            
        }
        case 0x02: {
            TCP_DISCONNECT_MSG discMsg;// = reinterpret_cast<TCP_DISCONNECT_MSG*>(state->buffer_recv);
            discMsg.set_msg(buffer);

            msg_source = discMsg.msg.source;
            //does stuff
            break;
        }
        case 0x03: {
            TCP_UPDATE_MESSAGE updMsg;// = reinterpret_cast<TCP_UPDATE_MESSAGE*>(state->buffer_recv);
            updMsg.set_msg(buffer);
            msg_source = updMsg.msg.source;
            //does stuff
            
            // These messages are to always be forwarded up the tree by sending to STA
            if(is_root == false) {
                send_to_sta = true;
            }
            
            // We must assume that the first one the parent gets will be from it's own children
            int count = updMsg.get_child_count();

            // Check to see if the source is a child
            if(client_tpcbs.find(msg_source) != client_tpcbs.end()) {
                                // Check source to make sure that the parent is in our tree somewhere
                if(tree.node_exists(msg_source) == false) {
                    DEBUG_printf("Node %d was not found in tree when it was expected to already be a child\n", msg_source);
                    while(true);
                }
                DEBUG_printf("Node being updated exists in tree, updating that node");
                // Add nodes to parent
                for (int i = 0; i < count; i++) {
                    // don't add the child if it's already in the tree
                    if(!tree.node_exists(updMsg.get_child(i))) {
                        tree.add_any_child(msg_source, updMsg.get_child(i));
                        SERIAL_NODE_ADD_MESSAGE serialMsg(msg_source, updMsg.get_child(i));
                        uart.sendMessage((char*)serialMsg.get_msg());
                    }
                }
            }
            

            break;
        }
        case 0x04: {
            DEBUG_printf("ACK msg received.\n");
            TCP_ACK_MESSAGE ackMsg; // = reinterpret_cast<TCP_ACK_MESSAGE*>(state->buffer_recv);
            ackMsg.set_msg(buffer);

            msg_source = ackMsg.msg.source;
            //does stuff
            // Do not need to respond to acks
            if (ackMsg.msg.dest == get_NodeID()) {
                //rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                DEBUG_printf("ACK was for me.\n");
                send_to_sta = true;;
                ACK_flag = false;
            } else {
                DEBUG_printf("ACK was for someone\n");
                uint32_t dest;
                DEBUG_printf("Message was not for me, was for dest:%08x\n", ackMsg.msg.dest);
                if(tree.find_path_parent(ackMsg.msg.dest, &dest)) {
                    DEBUG_printf("Sending down the tree");
                    send_msg(buffer);
                } else {
                    DEBUG_printf("Sending to up the tree");
                    send_to_sta = true;
                }
                ACK_flag = false;
                //send_tcp_data(ackMsg.msg.dest, client_tpcbs.at(ackMsg.msg.dest), ackMsg.get_msg(), ackMsg.get_len());
            }
            


            break;
        }
        case 0x05: {
            TCP_NAK_MESSAGE nakMsg; // = reinterpret_cast<TCP_NAK_MESSAGE*>(state->buffer_recv);
            nakMsg.set_msg(buffer);

            msg_source = nakMsg.msg.source;

            ACK_flag = false;
            //does stuff
            break;
        }
        default:
            DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
            ACK_flag = false;
            break;
    }

    if (ACK_flag){
       DEBUG_printf("Sending ACK message to client %08x\n", msg_source);
        // Assumption: clients_map[tpcb].id implies that any message worth acking isn't being forwarded and is originating from the intended node
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), msg_source, p->tot_len);
        send_msg(ackMsg.get_msg());
        //send_tcp_data(ackMsg.get_msg(), ackMsg.get_len());
        DEBUG_printf("Sent ACK message to client %08x\n", msg_source);
        
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from clients_map and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), msg_source, p->tot_len);
        nakMsg.set_error(error);
        // send_tcp_data(clients_map[tpcb].id, tpcb, nakMsg.get_msg(), nakMsg.get_len());
        //send_tcp_data(nakMsg.get_msg(), nakMsg.get_len());
        send_msg(nakMsg.get_msg());
        return false;
    } else if (send_to_sta) {
        DEBUG_printf("Sending to STA");
        // depending on the type of data forward only what is necessary
        SERIAL_DATA_MESSAGE msg;
        msg.add_message(buffer, p->tot_len);
        uart.sendMessage((const char*)msg.get_msg());
    }

    return true;
}

err_t APNode::handle_transfering_data(uint8_t *buffer) {
    DEBUG_printf("Entered handle_transfering_data\n");

    uint8_t msg_id = buffer[1];
    switch (msg_id) {
        case 0x01: // data msg
        {
            TCP_DATA_MSG msg;
            msg.set_msg(buffer);

            send_msg(msg.get_msg());
            break;
        }
        case 0x02: 
        {
            
            break;
        }
        case 0x03: // update 
        {
            TCP_UPDATE_MESSAGE msg;
            msg.set_msg(buffer);

            send_msg(msg.get_msg());
            break;
        }
        case 0x04:
        {
            TCP_ACK_MESSAGE msg;
            msg.set_msg(buffer);

            send_msg(msg.get_msg());
            break;
        }
        case 0x05:
        {
            TCP_NAK_MESSAGE msg;
            msg.set_msg(buffer);

            send_msg(msg.get_msg());
            break;
        }
        default:
            break;
    
    }


    return 0;
}

err_t APNode::send_data(uint32_t send_id, ssize_t len, uint8_t *buf) {
    //TODO: Add tree and look whether it needs to go up or down the tree
    TCP_DATA_MSG msg(get_NodeID(), send_id);
    msg.add_message(buf, len);
    // if (client_tpcbs.find(send_id) == client_tpcbs.end()){
    //     return -2;
    // } 
    // tcp_pcb *client_pcb = client_tpcbs.at(send_id);
    // if (!send_tcp_data(send_id, client_pcb, msg.get_msg(), msg.get_len()))
    //     return -1;
    send_msg(msg.get_msg());
    return 0;
}

err_t APNode::send_msg(uint8_t* msg) {
    //TCP_MESSAGE* tcp_msg = parseMessage(msg);
    DEBUG_printf("Sending msg w/ len: %d", msg[2]);
    DUMP_BYTES(msg, msg[2]);
    
    size_t len = 0;
    uint32_t target_id = 0;
    tcp_pcb *target = nullptr;

    uint8_t id = msg[1];
    switch (id) {
        case 0x00:  // Init message from STA -> thus new parent has been added
        {           // This message should probably never be received via serial
            // TCP_INIT_MESSAGE* initMsg = reinterpret_cast<TCP_INIT_MESSAGE*>(msg);
            // parent = initMsg->msg.source;

            TCP_INIT_MESSAGE initMsg;
            initMsg.set_msg(msg);

            

            return 0;
        }
        case 0x01: /* TCP_DATA_MSG */
        {
            TCP_DATA_MSG dataMsg;
            dataMsg.set_msg(msg);
            len = dataMsg.get_len();
            target_id = dataMsg.msg.dest;
            DUMP_BYTES(dataMsg.get_msg(), len);
            break;
        }
        case 0x02: /* TCP_DISCONNECT_MSG */
            // Sending this seems meaningless -> just update parent via STA

            //TCP_DISCONNECT_MSG* killMsg = reinterpret_cast<TCP_DISCONNECT_MSG*>(msg);
            //len = killMsg->get_len();
            //target_id = killMsg->msg.;
            break;
        case 0x03: /* TCP_UPDATE_MESSAGE */
        {
            TCP_UPDATE_MESSAGE updateMsg;
            updateMsg.set_msg(msg);
            len = updateMsg.get_len();
            target_id = updateMsg.msg.dest;
            break;
        }
        case 0x04: /* TCP_ACK_MESSAGE */
        {   
            TCP_ACK_MESSAGE ackMsg;
            ackMsg.set_msg(msg);
            len = ackMsg.get_len();
            target_id = ackMsg.msg.dest;
            break;
        }
        case 0x05: /* TCP_NAK_MESSAGE */
        {
            TCP_NAK_MESSAGE nakMsg;
            nakMsg.set_msg(msg);
            len = nakMsg.get_len();
            target_id = nakMsg.msg.dest;
            break;
        }
        case 0x06: /* TCP_FORCE_UPDATE_MESSAGE */
        {
            TCP_FORCE_UPDATE_MESSAGE fUpdateMsg;
            fUpdateMsg.set_msg(msg);
            len = fUpdateMsg.get_len();
            target_id = fUpdateMsg.msg.dest;
            break;
        }
        default:
        {
            return -1;
        } 
    }

    uint32_t path_parent;
    if(!tree.find_path_parent(target_id, &path_parent)) {
        ERROR_printf("Failed to find path parent");
        return -1;
    }
    
    if (client_tpcbs.count(path_parent) == 0) {
        ERROR_printf("Failed to find pcb* to client");
        return -1;
    }
    target = client_tpcbs.at(path_parent);
    DEBUG_printf("Sending msg");
    DUMP_BYTES(msg, sizeof(msg));
    send_tcp_data(target_id, target, msg, len);
    
    return 0;
}

err_t APNode::handle_serial_message(uint8_t *msg) {
    uint8_t id = serialMessageType(msg);
    DEBUG_printf("Got serial message\n");
    DUMP_BYTES(msg, msg[1]);

    // TODO: Finish switch-case
    switch (id) {
        case 0x00: /* serial_node_add_msg */
        {
            // If AP receives node_add, a parent has been added, maybe?
            SERIAL_NODE_ADD_MESSAGE serial_msg;
            serial_msg.set_msg(msg);

            // TODO: figure which to set parent = to
            parent = serial_msg.get_parent();
            break; 
        }
        case 0x01: /* serial_node_remove_msg */
            // If AP receives node_remove, a parent has been removed
            parent = UINT32_MAX;
            break;
        case 0x02: /* Data message */
        {
            DEBUG_printf("Got DATA SERIAL MESSAGE\n");
            SERIAL_DATA_MESSAGE serial_msg;
            serial_msg.set_msg(msg);

            handle_transfering_data(serial_msg.get_data());
            
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
