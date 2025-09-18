#include "MeshNode.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"

#if DEBUG 
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#if DEBUG
 static void dump_bytes(const uint8_t *bptr, uint32_t len) {
     unsigned int i = 0;
 
    DEBUG_printf("dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
            DEBUG_printf("\n");
         } else if ((i & 0x07) == 0) {
            DEBUG_printf(" ");
         }
        DEBUG_printf("%02x ", bptr[i++]);
     }
    DEBUG_printf("\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif

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
    uint8_t error = 0;
    // tcp_init_msg_t *init_msg_str = reinterpret_cast <tcp_init_msg_t *>(state->buffer_recv);
    // clients_map[tpcb].id = init_msg_str->source;
    //DEBUG_printf("ID RECV FROM INIT MESSAGE: %08X\n", clients_map[tpcb].id);
    // uint32_t test = ((APNode*)(state->ap_node))->get_NodeID();
    //DEBUG_printf("CURRENT NODE ID: %08X\n", test);

    puts("HANDLE DUMP");
    DUMP_BYTES(buffer, 100); 

    // TCP_INIT_MESSAGE init_msg(((APNode*)(state->ap_node))->get_NodeID());  
    //TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(state->buffer_recv));
    uint8_t msg_id = 0xFF;

    // if (!msg) {
    //    DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
    //     ACK_flag = false;
    // } else {

    // TODO: Handle error checks for messages
    msg_id = buffer[1];

        //DUMP_BYTES(state->buffer_recv, 2048);

    switch (msg_id) {
        case 0x00: {
            TCP_INIT_MESSAGE* initMsg = reinterpret_cast<TCP_INIT_MESSAGE*>(state->buffer_recv);
            DEBUG_printf("Received initialization message from node %u", initMsg->msg.source);
            //does stuff

            // Set init node ID
            if(clients_map[tpcb].id_recved == false) {
                ACK_flag = true;
                clients_map[tpcb].id_recved = true;
                clients_map[tpcb].id = initMsg->msg.source;
            }

            tree.add_child(initMsg->msg.source);

            // Insert into map of IDs to TPCB
            client_tpcbs.insert({clients_map[tpcb].id, tpcb});

            update_flag = true;
            
            break;
        }
        case 0x01: {
            TCP_DATA_MSG* dataMsg = reinterpret_cast<TCP_DATA_MSG*>(state->buffer_recv);
            // Store messages in a ring buffer
            puts("Got data message");
            DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
            if (dataMsg->msg.dest == get_NodeID()) {
                rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                DEBUG_printf("Successfully inserted into ring buffer\n");
                ACK_flag = true;
                break;
            } else {
                uint32_t dest;
                DEBUG_printf("Message was not for me, was for dest:%08x\n", dataMsg->msg.dest);
                // if(!tree.find_path_parent(dataMsg->msg.dest, &dest)) {
                //     DEBUG_printf("Count not find path to parent\n");
                //     ACK_flag = false;
                //     //NAK_flag = true;
                //     error = 0x01; // TODO make enum for errors (no node in tree)
                //     TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                //     nakMsg.set_error(error);
                //     send_tcp_data(dataMsg->msg.source, client_tpcbs.at(dataMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                //     break;
                // }
                // DEBUG_printf("Found path to parent, starts with %u\n", dest);
                if (client_tpcbs.find(dataMsg->msg.dest) == client_tpcbs.end()) {
                    ACK_flag = false;
                    //NAK_flag = true;
                    error = 0x02; // TODO make enum for errors (id not in connected clients)
                    TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                    nakMsg.set_error(error);
                    send_tcp_data(dataMsg->msg.source, client_tpcbs.at(dataMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                    break;
                }
                ACK_flag = false;
                DEBUG_printf("Forwarding message to node\n");
                send_tcp_data(dataMsg->msg.dest, client_tpcbs.at(dataMsg->msg.dest), dataMsg->get_msg(), dataMsg->get_len());
                break;
            }
            
        }
        case 0x02: {
            TCP_DISCONNECT_MSG* discMsg = reinterpret_cast<TCP_DISCONNECT_MSG*>(state->buffer_recv);
            //does stuff
            break;
        }
        case 0x03: {
            TCP_UPDATE_MESSAGE* updMsg = reinterpret_cast<TCP_UPDATE_MESSAGE*>(state->buffer_recv);
            //does stuff
            break;
        }
        case 0x04: {
            TCP_ACK_MESSAGE* ackMsg = reinterpret_cast<TCP_ACK_MESSAGE*>(state->buffer_recv);
            //does stuff
            // Do not need to respond to acks
            if (ackMsg->msg.dest == get_NodeID()) {
                //rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                ACK_flag = false;
            } else {
                uint32_t dest;
                DEBUG_printf("Message was not for me, was for dest:%08x\n", ackMsg->msg.dest);
                // if(!tree.find_path_parent(ackMsg->msg.dest, &dest)) {
                //     ACK_flag = false;
                //     NAK_flag = true;
                //     error = 0x01; // TODO make enum for errors (no node in tree)
                //     break;
                // }
                if (client_tpcbs.find(ackMsg->msg.dest) == client_tpcbs.end()) {
                    ACK_flag = false;
                    //NAK_flag = true;
                    error = 0x02; // TODO make enum for errors (id not in connected clients)
                    TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
                    nakMsg.set_error(error);
                    send_tcp_data(ackMsg->msg.source, client_tpcbs.at(ackMsg->msg.source), nakMsg.get_msg(), nakMsg.get_len());
                    break;
                }
                ACK_flag = false;
                send_tcp_data(ackMsg->msg.dest, client_tpcbs.at(ackMsg->msg.dest), ackMsg->get_msg(), ackMsg->get_len());
            }
            


            break;
        }
        case 0x05: {
            TCP_NAK_MESSAGE* nakMsg = reinterpret_cast<TCP_NAK_MESSAGE*>(state->buffer_recv);
            ACK_flag = false;
            //does stuff
            break;
        }
        default:
            DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
            ACK_flag = false;
            break;
    }
    // }

    if (update_flag) {
        puts("Update nodes called");
        /*puts("Broadcasting all connected nodes to each other");
        uint32_t ids[4] = {0};
        int j = 0;

        // Because this is a new node, resend the list of connected nodes to everyone
        for(auto i : client_tpcbs) {
            ids[j] = i.first;
            j++;
        }

        TCP_UPDATE_MESSAGE updateMsg(get_NodeID());
        updateMsg.add_children(j, ids);

        for(auto i : client_tpcbs) {
            sleep_ms(20);
            send_tcp_data(i.first, i.second, updateMsg.get_msg(), updateMsg.get_len());
           DEBUG_printf("Sent update message to %08x\n", i.first);
        }*/
    }

    if (ACK_flag){
       DEBUG_printf("Sending ACK message to client %08x\n", clients_map[tpcb].id);
        // Assumption: clients_map[tpcb].id implies that any message worth acking isn't being forwarded and is originating from the intended node
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
        send_tcp_data(clients_map[tpcb].id, tpcb, ackMsg.get_msg(), ackMsg.get_len());
        //send_tcp_data(ackMsg.get_msg(), ackMsg.get_len());
       DEBUG_printf("Sent ACK message to client %08x\n", clients_map[tpcb].id);
        
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from clients_map and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), clients_map[tpcb].id, p->tot_len);
        nakMsg.set_error(error);
        send_tcp_data(clients_map[tpcb].id, tpcb, nakMsg.get_msg(), nakMsg.get_len());
        //send_tcp_data(nakMsg.get_msg(), nakMsg.get_len());
        return false;
    } 

    return true;
}


err_t APNode::send_data(uint32_t send_id, ssize_t len, uint8_t *buf) {
    //TODO: Add tree and look whether it needs to go up or down the tree
    TCP_DATA_MSG msg(get_NodeID(), send_id);
    msg.add_message(buf, len);
    if (client_tpcbs.find(send_id) == client_tpcbs.end()){
        return -2;
    } 
    tcp_pcb *client_pcb = client_tpcbs.at(send_id);
    if (!send_tcp_data(send_id, client_pcb, msg.get_msg(), msg.get_len()))
        return -1;
    return 0;
}

err_t APNode::send_msg(uint8_t* msg) {
    //TCP_MESSAGE* tcp_msg = parseMessage(msg);
    size_t len = 0;
    uint32_t target_id = 0;
    tcp_pcb *target = nullptr;

    uint8_t id = msg[1];
    switch (id) {
        case 0x00:  // Init message from STA -> thus new parent has been added
        {           // This message should probably never be received via serial
            TCP_INIT_MESSAGE* initMsg = reinterpret_cast<TCP_INIT_MESSAGE*>(msg);
            parent = initMsg->msg.source;

            return 0;
            /* TODO: This is for STA
                
                // len = initMsg->get_len();
                // target_id = initMsg->msg.source; // ??

                // Will have to send Update Message to parent
                tree.add_any_child(get_node_id(), initMsg->msg.source);
                

                uint32_t children[4];
                uint8_t children_count = 0;
                if (!tree.get_children(get_node_id(), children, children_count))
                {
                    //idk die or something
                    // TODO: failure states
                }

                TCP_UPDATE_MESSAGE updateMsg(get_node_id());
                updateMsg.add_children(children_count, children);

                target_id = 
            */
        }
        case 0x01: /* TCP_DATA_MSG */
        {
            TCP_DATA_MSG* dataMsg = reinterpret_cast<TCP_DATA_MSG*>(msg);
            len = dataMsg->get_len();
            target_id = dataMsg->msg.dest;

            break;
        }
        case 0x02:
            break;
        case 0x03:
            break;
        default:
        {
            return -1;
        }

        // TODO: target = find(target_id);
        uint32_t parent;
        tree.find_path_parent(target_id, &parent);
        target = client_tpcbs.at(parent);
        send_tcp_data(target_id, target, msg, len);
        
    }
    return 0;
}

err_t APNode::handle_serial_message(uint8_t *msg) {
    uint8_t id = serialMessageType(msg);

    // TODO: Finish switch-case
    switch (id) {
        case 0x00: /* serial_node_add_msg */
        {
            // If AP receives node_add, a parent has been added, maybe?
            SERIAL_NODE_ADD_MESSAGE serial_msg;
            serial_msg.set_msg(msg);

            // TODO: figure which to set parent = to
            break; 
        }
        case 0x01: /* serial_node_remove_msg */
            // If AP receives node_remove, a parent has been removed
            break;
        case 0x02: /* Data message */
        {
            // SERIAL_DATA_MESSAGE serial_msg;
            // serial_msg.set_msg(msg);

            send_msg(msg);
            
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