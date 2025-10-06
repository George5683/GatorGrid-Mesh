#include "MeshNode.hpp"
#include <cstdint>
#include <lwip/opt.h>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "Messages.hpp"
#include "RingBuffer.hpp"
#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "display.hpp"



bool STANode::send_tcp_data(uint8_t* data, uint32_t size, bool forward) {

    // uint8_t buffer[size] = {};
    // if (size > BUF_SIZE) { size = size; }
    // memcpy(buffer, data, size);

    bool flag = false;

    // while(state->tcp_pcb->snd_buf != 0) {
    //  DEBUG_printf("tcp buffer has %d bytes in it\n", state->tcp_pcb->snd_buf);
    // }
    if (!forward) {
        while(state->waiting_for_ack); //{sleep_ms(5);}
    }
    //sleep_ms(10);
    cyw43_arch_lwip_begin();
    //printf("Space used in the buffer %d\n", tcp_sndbuf(state->tcp_pcb));
    // while (tcp_write(state->tcp_pcb, (void*)buffer, BUF_SIZE, TCP_WRITE_FLAG_COPY) == -1) {
    //    DEBUG_printf("attempting to write\n");
    //     sleep_ms(2);
    // }

    // Not getting an ack message back for some reason
    //sleep_ms(125);
    
    DEBUG_printf("Before tpc_write\n");

    err_t err = tcp_write(state->tcp_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);

    DEBUG_printf("After tpc_write\n");

    err_t err2 = tcp_output(state->tcp_pcb);

    DEBUG_printf("After tpc_output\n");


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
    
    DEBUG_printf("SENDING BYTES BELOW: \n");
    state->waiting_for_ack = !forward;
    DUMP_BYTES(data, size);
    return true;
}



/**
 * @brief Will wait until availble to send tcp, then will wait for an ACK response before releasing
 * 
 * @param data 
 * @param size 
 * @param forward 
 * @return true - message successfully sent
 * @return false - message failed to send
 */

bool STANode::send_tcp_data_blocking(uint8_t* data, uint32_t size, bool forward) {

    bool flag = false;

    while(state->waiting_for_ack); 
    cyw43_arch_lwip_begin();

    DEBUG_printf("blocking before write/output\n");
    err_t err = tcp_write(state->tcp_pcb, (void*)data, size, TCP_WRITE_FLAG_COPY);
    err_t err2 = tcp_output(state->tcp_pcb);
    DEBUG_printf("blocking after write/output\n");
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
    if (flag) {
        DEBUG_printf("Some error in write/output\n");
      return false;
    }
    
    DEBUG_printf("SENDING BYTES BELOW: \n");
    DUMP_BYTES(data, size);
    state->waiting_for_ack = !forward;
    
    int count = 0;
    while(state->waiting_for_ack)
    {
        sleep_ms(2);
        printf(".");
        count++;
        if(count == 1000) {
            state->waiting_for_ack = false;
            DEBUG_printf("\nTimeout\n");
            return false;
        }
            
    }
    if (state->got_nak) {
        DEBUG_printf("Got NAK, returning false\n");
        return false;
    }
    return true;
}

bool STANode::handle_incoming_data(unsigned char* buffer, uint16_t tot_len) {
    uint32_t source_id = 0;
    bool ACK_flag = false;
    bool NAK_flag = false;
    bool self_reply = false;
    bool send_to_ap = false;
    state->got_nak = false;
    
    // TCP_MESSAGE* msg = parseMessage(reinterpret_cast <uint8_t *>(buffer));
    // if (!msg) {
    //    DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
    //     ACK_flag = false;
    // } else {

    uint8_t msg_id = buffer[1];
    uint16_t len = *reinterpret_cast<uint16_t*>(buffer +2);
    DEBUG_printf("DUMPING STATE BUFFER BELOW:\n");
    DUMP_BYTES(state->buffer, 200);
    DEBUG_printf("DUMPING BYTES RECV BELOW:\n");
    DUMP_BYTES(buffer, 200);
    //dump_bytes(buffer, 100);
    switch (msg_id) {
        case 0x00: {
            TCP_INIT_MESSAGE initMsg;// = reinterpret_cast<TCP_INIT_MESSAGE*>(buffer);
            initMsg.set_msg(buffer);
            //does stuff
            DEBUG_printf("Received initialization message from node %u", initMsg.msg.source);
            source_id =  initMsg.msg.source;

            tree.add_child(source_id);

            break;
        }
        case 0x01: {
            TCP_DATA_MSG dataMsg;
            dataMsg.set_msg(buffer);

            DEBUG_printf("Got data message\n");
            if (dataMsg.msg.dest == get_NodeID()) {
                source_id =  dataMsg.msg.source;
                rb.insert(dataMsg.msg.msg,dataMsg.msg.msg_len, dataMsg.msg.source, dataMsg.msg.dest);
                DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg.msg.msg_len, dataMsg.msg.source, dataMsg.msg.dest);
                DEBUG_printf("DBG: Received message from node %08x: %s", dataMsg.msg.source, (char*)dataMsg.msg.msg);
                ACK_flag = true;
                break;
            } else {
                uint32_t path = 0;
                if (tree.find_path_parent(dataMsg.msg.dest, &path)) { // if true, needs to be passed on to AP
                    send_to_ap = true;
                } else {
                    send_msg(dataMsg.get_msg());
                }
            }
            //does stuff
            break;
        }
        case 0x02: {
            TCP_DISCONNECT_MSG initMsg;// = reinterpret_cast<TCP_DISCONNECT_MSG*>(buffer);
            initMsg.set_msg(buffer);
            
            //does stuff
            break;
        }
        case 0x03: {
            TCP_UPDATE_MESSAGE updateMsg;// = reinterpret_cast<TCP_UPDATE_MESSAGE*>(buffer);
            updateMsg.set_msg(buffer);

            DEBUG_printf("Update packet recieved\n");

            uint32_t id_buffer[4];
            MEMCPY(id_buffer, updateMsg.msg.children_IDs, sizeof(id_buffer));

            if(!tree.update_node(updateMsg.msg.source, id_buffer, updateMsg.msg.child_count)) {
                ERROR_printf("Failed to update node");
            }

            //does stuff
            break;
        }
        case 0x04: {
            DEBUG_printf("Got ACK\n");
            TCP_ACK_MESSAGE ackMsg;// = reinterpret_cast<TCP_ACK_MESSAGE*>(buffer);
            ackMsg.set_msg(buffer);

            source_id = ackMsg.msg.source;
            uint32_t path_parent = 0;

            DEBUG_printf("Ack is from %08x and for %08x\n", ackMsg.msg.source, ackMsg.msg.dest);
            if (ackMsg.msg.dest == get_NodeID()) {
                self_reply = true;
                DEBUG_printf("ACK is for me\n");
                state->waiting_for_ack = false;
            } else if (tree.find_path_parent(ackMsg.msg.dest, &path_parent)) {
                send_to_ap = true;
            } else {
                send_msg(buffer);
            }
            //does stuff
            break;
        }
        case 0x05: {
            TCP_NAK_MESSAGE nakMsg;// = reinterpret_cast<TCP_NAK_MESSAGE*>(buffer);
            nakMsg.set_msg(buffer);

            //does stuff
            DEBUG_printf("Warning! Message was not received! (Got a NAK instead of ACK)");
            DEBUG_printf("Got NAK\n");
            self_reply = true;
            state->waiting_for_ack = false;
            state->got_nak = true;
            break;
        }
        default:
            DEBUG_printf("Error: Unable to parse message (invalid buffer or unknown msg_id).\n");
            ACK_flag = false;
            // SEND NAK message
            //TCP_NAK_MESSAGE nakMsg(node->get_NodeID(), msg_id ? msg_id : 0, 0);
            break;
    }

    if (send_to_ap) {
        DEBUG_printf("Transferring message to AP node");
        uint8_t msg_id = buffer[1];

        SERIAL_DATA_MESSAGE msg;
        msg.add_message(buffer, buffer[2]);

        uart.sendMessage((char*) msg.get_msg());
        //TCP_NAK_MESSAGE* nakMsg = static_cast<TCP_NAK_MESSAGE*>(msg); 


        // uart.sendMessage();
        return true;
    }

    if (ACK_flag && !self_reply){
        DEBUG_printf("Sending ack to %08x\n", source_id);
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), source_id, tot_len);
        send_tcp_data(ackMsg.get_msg(), ackMsg.get_len(), true);
        //state->waiting_for_ack = true;
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from sender and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), 0, tot_len);
        send_tcp_data(nakMsg.get_msg(), nakMsg.get_len(), true);
        // delete msg;
        return false;
    }

    // delete msg;
    return true;
}

err_t STANode::send_data(uint32_t send_id, ssize_t len, uint8_t *buf) {
    TCP_DATA_MSG msg(get_NodeID(), send_id);
    msg.add_message(buf, len);
    if (!send_tcp_data_blocking(msg.get_msg(), msg.get_len(), false))
        return -1;
    return 0;
}

err_t STANode::send_msg(uint8_t* msg) {
    //TCP_MESSAGE* tcp_msg = parseMessage(msg);
    
    size_t len = 0;
    uint32_t target_id = 0;
    tcp_pcb *target = nullptr;

    bool forward = false;

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

            if (dataMsg.msg.source != get_NodeID()) forward = true;
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
            DEBUG_printf("TCP_UPDATE_MESSAGE being sent");
            TCP_UPDATE_MESSAGE updateMsg;
            updateMsg.set_msg(msg);
            len = updateMsg.get_len();
            target_id = updateMsg.msg.dest;

            if (updateMsg.msg.source != get_NodeID()) {
                DEBUG_printf("Update message was passing through");
                forward = true;
            }
            break;
        }
        case 0x04: /* TCP_ACK_MESSAGE */
        {   
            TCP_ACK_MESSAGE ackMsg;
            ackMsg.set_msg(msg);
            len = ackMsg.get_len();
            target_id = ackMsg.msg.dest;

            if (ackMsg.msg.source != get_NodeID()) forward = true;
            break;
        }
        case 0x05: /* TCP_NAK_MESSAGE */
        {
            TCP_NAK_MESSAGE nakMsg;
            nakMsg.set_msg(msg);
            len = nakMsg.get_len();
            target_id = nakMsg.msg.dest;

            if (nakMsg.msg.source != get_NodeID()) forward = true;
            break;
        }
        case 0x06: /* TCP_FORCE_UPDATE_MESSAGE */
        {
            TCP_FORCE_UPDATE_MESSAGE fUpdateMsg;
            fUpdateMsg.set_msg(msg);
            len = fUpdateMsg.get_len();
            target_id = fUpdateMsg.msg.dest;

            forward = true;
            break;
        }
        default:
        {
            return -1;
        }
    }

    DUMP_BYTES(msg, len);
    uint32_t path_parent = 0;
    if (!tree.find_path_parent(target_id, &path_parent)) {
        DEBUG_printf(forward ? "Forwarding message" : "Expecting ACK");
        if (!send_tcp_data_blocking(msg, len, forward)) {
            ERROR_printf("send_tcp_data_blocking failed");
            return -1;
        }
    } else {
        SERIAL_DATA_MESSAGE serialMsg;
        serialMsg.add_message(msg, len);
        uart.sendMessage((char*)serialMsg.get_msg());
    }
    DEBUG_printf("Message successfully sent");
    
    return 0;
}

err_t STANode::handle_serial_message(uint8_t *msg) {
    uint8_t id = serialMessageType(msg);
    DEBUG_printf("Received serial message in handler\n");
    DUMP_BYTES(msg, msg[1]);

    // TODO: Finish switch-case
    switch (id) {
        case 0x00: /* serial_node_add_msg */
        {
            DEBUG_printf("Received serial node add message");
            // If STA receives node_add, a child has been added.
            SERIAL_NODE_ADD_MESSAGE initMsg;
            initMsg.set_msg(msg);
            
            tree.add_any_child(initMsg.msg.parent, initMsg.msg.child);
            DEBUG_printf("Child successfully added");

            uint32_t children[4];
            uint8_t children_count = 0;
            if (!tree.get_children(initMsg.msg.parent, children, children_count))
            {
                DEBUG_printf("Get children failed :(");
                return -1;
                //idk die or something
                // TODO: failure states
            }

            DEBUG_printf("Sending Update Msg");
            TCP_UPDATE_MESSAGE updateMsg(initMsg.msg.parent, parent);
            updateMsg.add_children(children_count, children);

            return(send_msg(updateMsg.get_msg()));
        }
        case 0x01: /* serial_node_remove_msg */
            // If AP receives node_remove, a parent has been removed
            break;
        case 0x02: /* Data message */
        {
            DEBUG_printf("Got DATA SERIAL MESSAGE\n");
            SERIAL_DATA_MESSAGE serial_msg;
            serial_msg.set_msg(msg);

            handle_incoming_data(serial_msg.get_data(),serial_msg.get_data_len());
            
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
