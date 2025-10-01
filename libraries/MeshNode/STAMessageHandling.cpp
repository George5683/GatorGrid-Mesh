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
        count++;
        if(count == 1000) {
            state->waiting_for_ack = false;
            DEBUG_printf("Timeout\n");
            return false;
        }
            
    }
    if (state->got_nak) {
        DEBUG_printf("Got NAK, returning false\n");
        return false;
    }
    return true;
}

bool STANode::handle_incoming_data(unsigned char* buffer, struct pbuf *p) {
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
                TCP_INIT_MESSAGE* initMsg = reinterpret_cast<TCP_INIT_MESSAGE*>(buffer);
                //does stuff
               DEBUG_printf("Received initialization message from node %u", initMsg->msg.source);
                source_id =  initMsg->msg.source;

                break;
            }
            case 0x01: {
                TCP_DATA_MSG* dataMsg = reinterpret_cast<TCP_DATA_MSG*>(buffer);
                DEBUG_printf("Got data message\n");
                if (dataMsg->msg.dest == get_NodeID()) {
                    source_id =  dataMsg->msg.source;
                    rb.insert(dataMsg->msg.msg,dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    DEBUG_printf("Testing dataMsg, len:%u, source:%08x, dest:%08x\n",dataMsg->msg.msg_len, dataMsg->msg.source, dataMsg->msg.dest);
                    DEBUG_printf("DBG: Received message from node %08x: %s", dataMsg->msg.source, (char*)dataMsg->msg.msg);
                    ACK_flag = true;
                    break;
                } else {
                    // TODO pass msg to AP
                    send_to_ap = true;

                }
                //does stuff
                break;
            }
            case 0x02: {
                TCP_DISCONNECT_MSG* initMsg = reinterpret_cast<TCP_DISCONNECT_MSG*>(buffer);
                
                //does stuff
                break;
            }
            case 0x03: {
                TCP_UPDATE_MESSAGE* initMsg = reinterpret_cast<TCP_UPDATE_MESSAGE*>(buffer);


                //puts("Update packet recieved");

                //updMsg.

                //does stuff
                break;
            }
            case 0x04: {
                DEBUG_printf("Got ACK\n");
                TCP_ACK_MESSAGE* ackMsg = reinterpret_cast<TCP_ACK_MESSAGE*>(buffer);

                source_id = ackMsg->msg.source;

               DEBUG_printf("Ack is from %08x and for %08x\n", ackMsg->msg.source, ackMsg->msg.dest);
                if (ackMsg->msg.dest == get_NodeID()) {
                    self_reply = true;
                    DEBUG_printf("ACK is for me\n");
                    state->waiting_for_ack = false;
                }
                //does stuff
                break;
            }
            case 0x05: {
                TCP_NAK_MESSAGE* nakMsg = reinterpret_cast<TCP_NAK_MESSAGE*>(buffer);
                //does stuff
                puts("Warning! Message was not received! (Got a NAK instead of ACK)");
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
        // }
    }

    if (send_to_ap) {

        uint8_t msg_id = buffer[1];

        // Different message types warrent different construction
        //switch (msg_id) {


        //}
        //TCP_NAK_MESSAGE* nakMsg = static_cast<TCP_NAK_MESSAGE*>(msg); 


        //uart.sendMessage();
        return true;
    }

    if (ACK_flag && !self_reply){
        DEBUG_printf("Sending ack to %08x\n", source_id);
        TCP_ACK_MESSAGE ackMsg(get_NodeID(), source_id, p->tot_len);
        send_tcp_data(ackMsg.get_msg(), ackMsg.get_len(), true);
        //state->waiting_for_ack = true;
    } else if (NAK_flag) {
        // TODO: Update for error handling
        // identify the source from sender and send back?
        TCP_NAK_MESSAGE nakMsg(get_NodeID(), 0, p->tot_len);
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

int handle_serial_message(uint8_t *msg) {
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

            //send_msg(msg);
            
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
