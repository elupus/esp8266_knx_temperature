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

#include <SmingCore.h>
#include "knx.h"

#define KNX_FIELD_HEADER_LEN 0x06u
#define KNX_FIELD_HEADER_VER 0x10u
#define KNX_FIELD_HOPCOUNT      6u

#define KNX_CONFIG_DEVICE_ADDRESS          KNX_DEVICE_ADDRESS(1, 2, 0)
#define KNX_CONFIG_DEVICE_HARDWARE         0u
#define KNX_CONFIG_DEVICE_SERVICE_FAMILIES 0u

void knx_recieved_data(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort);

UdpConnection      g_udp(knx_recieved_data);
IPAddress          g_target_address(224,   0, 23,  12);
const unsigned int g_target_port = 3671;

void set_u16(char buf[2], uint16_t val)
{
    buf[0u] = (val >> 8) & 0xffu;
    buf[1u] = (val >> 0) & 0xffu;
}

uint16_t get_u16(const char buf[2])
{
    return ((uint16_t)buf[0u] << 8u)
         | ((uint16_t)buf[1u] << 0u);
}

void set_u08(char buf[1], uint8_t val)
{
    buf[0] = val;
}

uint8_t get_u08(const char buf[1])
{
    return buf[0];
}

void set_ipv4(char buf[4], const IPAddress& address)
{
    set_u08(&buf[0u], address[0]);
    set_u08(&buf[1u], address[1]);
    set_u08(&buf[2u], address[2]);
    set_u08(&buf[3u], address[3]);
}

void knx_set_float16(uint8_t buf[2], float data)
{
    if (isnan(data)) {
        set_u16((char*)buf, 0x7FFF);
    } else {
        int   exp;
        float x = frexp(data, &exp);

        union {
            int16_t  s;
            uint16_t u;
        } m;
        m.s  = (int16_t)(0.5f + x * 100.0f);
        m.u &= 0b1000011111111111;
        m.u |= exp << 11;
        set_u16((char*)buf, m.u);
    }
}

void set_hpai(char buf[8], const IPAddress& ip, uint16_t port)
{
    set_u08(&buf[0u], 8u);                                 /* structure length */
    set_u08(&buf[1u], KNX_HOST_PROTOCOL_IPV4_UDP);
    set_ipv4(&buf[2u], ip);
    set_u16(&buf[6u], port);
}

uint8_t get_hpai(const char* buf, uint16 len, IPAddress& ip, uint16_t& port)
{
    if (len < 8u) {
        Serial.printf("HPAI: Buffer to small %u\r\n", len);
        return 0u;
    }

    /* len */
    if (get_u08(&buf[0]) != 8u) {
        Serial.printf("HPAI: Unsupported length %u\r\n", get_u08(&buf[0]));
        return 0u;
    }

    /* protocol */
    if (get_u08(&buf[1]) != KNX_HOST_PROTOCOL_IPV4_UDP) {
        Serial.printf("HPAI: Unsupported protocol %u\r\n", get_u08(&buf[1]));
        return 0u;
    }

    ip   = (uint8_t*)&buf[2];
    port = get_u16(&buf[6]);
    return 8u;
}

void set_device_info(char buf[54])
{
    set_u08(&buf[ 0u], 54u); /* structure length */
    set_u08(&buf[ 1u], KNX_DESCRIPTION_TYPE_DEVICE_INFO); /* description type code */
    set_u08(&buf[ 2u], KNX_DEVICE_MEDIUM_TP1);
    set_u08(&buf[ 3u], 0u); /* device status */
    set_u16(&buf[ 4u], KNX_CONFIG_DEVICE_ADDRESS); /* device status */
    set_u16(&buf[ 6u], 0u); /* project installation identifier */
    memset(&buf[8], 0, 6);  /* serial number */
    set_ipv4(&buf[14u], g_target_address);
    memset(&buf[18], 0, 6);  /* mac address */
    memset(&buf[24], 0, 30); /* friendly name */
    strcpy(&buf[24], "ESP Temperature Sensor");
}

void set_supported_service_familes(char buf[6])
{
    set_u08(&buf[ 0u], 6u); /* structure length */
    set_u08(&buf[ 1u], KNX_DESCRIPTION_TYPE_SUPP_SVC_FAMILIES); /* description type code */
    set_u08(&buf[ 2u], KNX_SERVICEFAMILIY_2);
    set_u08(&buf[ 3u], 5u); /* service family version */
    set_u08(&buf[ 4u], KNX_SERVICEFAMILIY_ROUTING);
    set_u08(&buf[ 5u], 5u); /* service family version */
}

void knx_send_group_write_f16(uint16 group, float val)
{
    uint8_t buf[3];
    buf[0] = 0u;
    knx_set_float16(&buf[1], val);
    knx_send_routing_indication( group
                            , 0u
                            , KNX_APCI_VALUE_WRITE
                            , buf
                            , sizeof(buf));
}

void knx_send_group_write_u01_to_u07(uint16 group, uint8_t val)
{
    uint8_t buf[1];
    buf[0] = val;
    knx_send_routing_indication( group
                            , 0u
                            , KNX_APCI_VALUE_WRITE
                            , buf
                            , sizeof(buf));
}

void knx_send_search_response(IPAddress ip, uint16 port)
{
    char buf[74];
    /* ip header */
    set_u08(&buf[ 0u], KNX_FIELD_HEADER_LEN);        /* fixed header length */
    set_u08(&buf[ 1u], KNX_FIELD_HEADER_VER);        /* fixed protocol version */
    set_u16(&buf[ 2u], KNX_SERVICE_SEARCH_RES);      /* service type */
    set_u16(&buf[ 4u], sizeof(buf));   /* total length */

    /* body */
    /* hpai host protocol address information */
    set_hpai(&buf[ 6u], WifiStation.getIP(), g_target_port);
    set_device_info(&buf[14u]);
    set_supported_service_familes(&buf[68u]);
    g_udp.sendTo(ip, port, buf, sizeof(buf));
}

void knx_send_routing_indication(uint16_t target, uint8_t tpci, uint16_t apci, uint8_t* data, uint8_t len)
{
    char buf[17 + 16u];
    /* ip header */
    set_u08(&buf[ 0u], KNX_FIELD_HEADER_LEN);       /* fixed header length */
    set_u08(&buf[ 1u], KNX_FIELD_HEADER_VER);       /* fixed protocol version */
    set_u16(&buf[ 2u], KNX_SERVICE_ROUTING_IND);    /* service type */
    set_u16(&buf[ 4u], sizeof(buf));                /* total length */

    /* body */
    /* body - connection */
    set_u08(&buf[ 6u], KNX_MESSAGE_CODE_LDATA_IND); /* message code */
    set_u08(&buf[ 7u], 00u);                        /* additonal information */

    /* body - cEMI */
    set_u08(&buf[ 8u], (KNX_FRAME_FORMAT_STD           << KNX_FIELD_CONTROL1_FORMAT_OFFSET)
                     | (1u                             << KNX_FIELD_CONTROL1_REPEAT_OFFSET)
                     | (1u                             << KNX_FIELD_CONTROL1_BROADCAST_OFFSET)
                     | (KNX_PRIORITY_NORMAL            << KNX_FIELD_CONTROL1_PRIO_OFFSET)
                     | (0u                             << KNX_FIELD_CONTROL1_CONFIRM_OFFSET));                  /* control field */
    set_u08(&buf[ 9u], (1u                             << KNX_FIELD_CONTROL2_DESTTYPE_OFFSET)
                     | (KNX_FIELD_HOPCOUNT             << KNX_FIELD_CONTROL2_HOPCOUNT_OFFSET)
                     | (KNX_EXTFRAME_FORMAT_STD        << KNX_FIELD_CONTROL2_EXTFRAME_OFFSET));     /* Typ Zieladr.(1b); Rout.Zähl.(3b); Ext. Frame Format(4b). */
    set_u16(&buf[10u], KNX_CONFIG_DEVICE_ADDRESS);  /* source address */
    set_u16(&buf[12u], target);                     /* target address */
    set_u08(&buf[14u], len);                        /* 0000 + Längenfeld(4b). */

    /* apci/highest data are intermixed */
    if (len > 0u) {
        apci |= data[0u];
        len--;
        data++;
    }

    set_u16(&buf[15u], (tpci << 8u)
                     | (apci << 0u));                    /* TPCI/APCI & data */
    if (len > 0u) {
        memcpy(&buf[17u], &data[0u], len);
    }
    g_udp.sendTo(g_target_address, g_target_port, buf, 17u + len);
}


void knx_recieved_routing_ind(char* data, int size)
{
    Serial.println("Routing indication");

    switch (get_u16(&data[6])) {
    case KNX_GROUP_ADDRESS(1, 1, 5):
        break;
    default:
        Serial.println("Unknown group address");
        break;
    }
}

void knx_recieved_search_req(char* data, int size)
{
    Serial.println("Search request");
    uint8_t   res;
    IPAddress ip;
    uint16_t  port;


    res = get_hpai(data, size, ip, port);
    if (res == 0u) {
        return;
    }

    knx_send_search_response(ip, port);
}

void knx_recieved_data(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort)
{
    Serial.print("data:");
    Serial.println(remoteIP.toString());
    if (size < 6) {
        Serial.println("Invalid len");
        return;
    }

    if (data[0] != KNX_FIELD_HEADER_LEN) {
        Serial.println("Invalid header len");
        return;
    }

    if (data[1] != KNX_FIELD_HEADER_VER) {
        Serial.println("Invalid header ver");
        return;
    }

    switch (get_u16(&data[2])) {
    case KNX_SERVICE_ROUTING_IND:
        knx_recieved_routing_ind(&data[6], size - 6u);
        break;
    case KNX_SERVICE_SEARCH_REQ:
        knx_recieved_search_req(&data[6], size - 6u);
        break;
    default:
        Serial.printf("Unknown service %x\r\n", get_u16(&data[2]));
        break;
    }
}

void knx_init(void)
{
    if (!g_udp.listenMulticast(g_target_address, g_target_port)) {
        Serial.println("Failed to listen to multicast");
    }
}
