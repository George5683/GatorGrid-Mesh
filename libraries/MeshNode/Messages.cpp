#include <cstdint>
#include <cstring>
#include <cstdio>
#include "Messages.hpp"


// TCP_MESSAGE* parseMessage(uint8_t* data) {
//     //printf("Called parseMessage()");
//     uint8_t msg_id = data[1];
//     size_t data_len = *(reinterpret_cast<uint16_t*>(data+2));

//     if (data_len < 4) {
//         return nullptr;
//     }

    

//     switch (msg_id) {
//         case 0x00: {  // TCP_INIT_MESSAGE
//             if (data_len < sizeof(TCP_MESSAGE::tcp_init_msg))
//                 return nullptr;
//             const auto* rawInit = reinterpret_cast<const TCP_MESSAGE::tcp_init_msg*>(data);
//             TCP_INIT_MESSAGE* msg = new TCP_INIT_MESSAGE(rawInit->source);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         case 0x01: {  // TCP_DATA_MSG
//             const auto* rawData = reinterpret_cast<const TCP_MESSAGE::tcp_data_msg*>(data);
//             TCP_DATA_MSG* msg = new TCP_DATA_MSG(rawData->source, rawData->dest);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         case 0x02: {  // TCP_DISCONNECT_MSG
//             if (data_len < sizeof(TCP_MESSAGE::tcp_disconnect_msg))
//                 return nullptr;
//             const auto* rawDisc = reinterpret_cast<const TCP_MESSAGE::tcp_disconnect_msg*>(data);
//             TCP_DISCONNECT_MSG* msg = new TCP_DISCONNECT_MSG(rawDisc->source);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         case 0x03: {  // TCP_UPDATE_MESSAGE
//             if (data_len < sizeof(TCP_MESSAGE::tcp_update_msg))
//                 return nullptr;
//             const auto* rawUpdate = reinterpret_cast<const TCP_MESSAGE::tcp_update_msg*>(data);
//             TCP_UPDATE_MESSAGE* msg = new TCP_UPDATE_MESSAGE(rawUpdate->source, rawUpdate);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         case 0x04: {  // TCP_ACK_MESSAGE
//             if (data_len < sizeof(TCP_MESSAGE::tcp_acknowledge_msg))
//                 return nullptr;
//             const auto* rawAck = reinterpret_cast<const TCP_MESSAGE::tcp_acknowledge_msg*>(data);
//             TCP_ACK_MESSAGE* msg = new TCP_ACK_MESSAGE(rawAck->source, rawAck->dest, rawAck->bytes_received);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         case 0x05: {  // TCP_NAK_MESSAGE
//             if (data_len < sizeof(TCP_MESSAGE::tcp_failed_msg))
//                 return nullptr;
//             const auto* rawAck = reinterpret_cast<const TCP_MESSAGE::tcp_failed_msg*>(data);
//             TCP_NAK_MESSAGE* msg = new TCP_NAK_MESSAGE(rawAck->source, rawAck->dest, rawAck->bytes_received);
//             msg->set_msg((void*)data);
//             return msg;
//         }
//         default:
//             // Unknown msg_id
//             return nullptr;
//     }
// }
