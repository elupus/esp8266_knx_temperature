/* Copyright (C) 2016 Joakim Plate
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef INCLUDE_KNX_H_
#define INCLUDE_KNX_H_


#define KNX_SERVICEFAMILIY_2                   0x2u
#define KNX_SERVICEFAMILIY_ROUTING             0x5u

#define KNX_SERVICE_SEARCH_REQ                 0x201
#define KNX_SERVICE_SEARCH_RES                 0x202
#define KNX_SERVICE_DESCRIPTION_REQ            0x203
#define KNX_SERVICE_DESCRIPTION_RES            0x204
#define KNX_SERVICE_CONNECT_REQ                0x205
#define KNX_SERVICE_CONNECT_RES                0x206
#define KNX_SERVICE_CONNECTIONSTATE_REQ        0x207
#define KNX_SERVICE_CONNECTIONSTATE_RES        0x208
#define KNX_SERVICE_DISCONNECT_REQ  521        0x209
#define KNX_SERVICE_DISCONNECT_RES  522        0x20A
#define KNX_SERVICE_DEVICE_CONFIGURATION_REQ   0x310
#define KNX_SERVICE_DEVICE_CONFIGURATION_ACK   0x311
#define KNX_SERVICE_TUNNELING_REQ              0x420
#define KNX_SERVICE_TUNNELING_ACK              0x421
#define KNX_SERVICE_ROUTING_IND                0x530
#define KNX_SERVICE_ROUTING_LOST_MSG           0x531

#define KNX_PRIORITY_SYSTEM                    0u
#define KNX_PRIORITY_HIGH                      1u
#define KNX_PRIORITY_ALARM                     2u
#define KNX_PRIORITY_NORMAL                    3u

#define KNX_APCI_VALUE_READ                    0b0000000000u
#define KNX_APCI_VALUE_RESPONSE                0b0001000000u
#define KNX_APCI_VALUE_WRITE                   0b0010000000u
#define KNX_APCI_MEMORY_WRITE                  0b1010000000u

#define KNX_HOST_PROTOCOL_IPV4_UDP             1u
#define KNX_HOST_PROTOCOL_IPV4_TCP             2u

#define KNX_DESCRIPTION_TYPE_DEVICE_INFO       0x01u
#define KNX_DESCRIPTION_TYPE_SUPP_SVC_FAMILIES 0x02u
#define KNX_DESCRIPTION_TYPE_IP_CONFIG         0x03u
#define KNX_DESCRIPTION_TYPE_IP_CUR_CONFIG     0x04u
#define KNX_DESCRIPTION_TYPE_KNX_ADDRESSES     0x05u
#define KNX_DESCRIPTION_TYPE_MFR_DATA          0xFEu

#define KNX_DEVICE_MEDIUM_TP1                  0x02u
#define KNX_DEVICE_MEDIUM_PL110                0x04u
#define KNX_DEVICE_MEDIUM_RF                   0x10u
#define KNX_DEVICE_MEDIUM_KNX_IP               0x20u

#define KNX_FRAME_FORMAT_EXT                   0u
#define KNX_FRAME_FORMAT_STD                   1u

#define KNX_EXTFRAME_FORMAT_STD                0u

#define KNX_MESSAGE_CODE_LDATA_CON             0x2Eu
#define KNX_MESSAGE_CODE_LDATA_IND             0x29u
#define KNX_MESSAGE_CODE_LDATA_REQ             0x11u

#define KNX_FIELD_CONTROL1_FORMAT_OFFSET       7u
#define KNX_FIELD_CONTROL1_REPEAT_OFFSET       5u
#define KNX_FIELD_CONTROL1_BROADCAST_OFFSET    4u
#define KNX_FIELD_CONTROL1_PRIO_OFFSET         2u
#define KNX_FIELD_CONTROL1_ACKREQ_OFFSET       1u
#define KNX_FIELD_CONTROL1_CONFIRM_OFFSET      0u

#define KNX_FIELD_CONTROL2_DESTTYPE_OFFSET     7u
#define KNX_FIELD_CONTROL2_HOPCOUNT_OFFSET     4u
#define KNX_FIELD_CONTROL2_EXTFRAME_OFFSET     0u

#define KNX_GROUP_ADDRESS(a, b, c) ((a) << 11u | (b) << 8 | (c))
#define KNX_DEVICE_ADDRESS(area, line, device) ((area) << 12u | (line) << 8u | (device))

void knx_init(void);
void knx_send_routing_indication(uint16_t target, uint8_t tpci, uint16_t apci, uint8_t* data, uint8_t len);
void knx_send_group_write_f16(uint16 group, float val);

#endif /* INCLUDE_KNX_H_ */
