#include <user_config.h>
#include <SmingCore.h>
#include <libraries/DS18S20/ds18s20.h>

Timer procTimer;
int helloCounter = 0;
UdpConnection g_udp;

DS18S20            g_temperature;

//IPAddress          g_target_address(192, 168,  4, 226);
IPAddress          g_target_address(224,   0, 23,  12);
const unsigned int g_target_port = 3671;

void networkOk()
{
    Serial.println("Connected to wifi");
    Serial.print("IP: ");
    Serial.println(WifiStation.getIP().toString());

    Serial.print("GW: ");
    Serial.println(WifiStation.getNetworkGateway().toString());
    g_udp.connect(g_target_address, g_target_port);
}

void set_u16(char buf[2], uint16_t val)
{
    buf[0u] = (val >> 0) & 0xffu;
    buf[1u] = (val >> 8) & 0xffu;
}

void set_u08(char buf[1], uint8_t val)
{
    buf[0] = val;
}

#define KNX_SERVICE_SEARCH_REQ               0x201
#define KNX_SERVICE_SEARCH_RES               0x202
#define KNX_SERVICE_DESCRIPTION_REQ          0x203
#define KNX_SERVICE_DESCRIPTION_RES          0x204
#define KNX_SERVICE_CONNECT_REQ              0x205
#define KNX_SERVICE_CONNECT_RES              0x206
#define KNX_SERVICE_CONNECTIONSTATE_REQ      0x207
#define KNX_SERVICE_CONNECTIONSTATE_RES      0x208
#define KNX_SERVICE_DISCONNECT_REQ  521      0x209
#define KNX_SERVICE_DISCONNECT_RES  522      0x20A
#define KNX_SERVICE_DEVICE_CONFIGURATION_REQ 0x310
#define KNX_SERVICE_DEVICE_CONFIGURATION_ACK 0x311
#define KNX_SERVICE_TUNNELING_REQ            0x420
#define KNX_SERVICE_TUNNELING_ACK            0x421
#define KNX_SERVICE_ROUTING_IND              0x530
#define KNX_SERVICE_ROUTING_LOST_MSG         0x531

#define KNX_PRIORITY_SYSTEM         0b00000000
#define KNX_PRIORITY_HIGH           0b00000100
#define KNX_PRIORITY_ALARM          0b00001000
#define KNX_PRIORITY_NORMAL         0b00001100

#define KNX_COMMAND_VALUE_READ      0b00000000
#define KNX_COMMAND_VALUE_RESPONSE  0b00000001
#define KNX_COMMAND_VALUE_WRITE     0b00000010
#define KNX_COMMAND_MEMORY_WRITE    0b00001010


#define KNX_FIELD_HEADER_LEN 0x06u
#define KNX_FIELD_HEADER_VER 0x10u

#define KNX_FRAME_FORMAT_EXT    0u
#define KNX_FRAME_FORMAT_STD    2u
#define KNX_FRAME_FORMAT_POLL   3u

#define KNX_EXTFRAME_FORMAT_STD 0u

#define KNX_FIELD_CONTROL1_FORMAT_OFFSET 6u
#define KNX_FIELD_CONTROL1_REPEAT_OFFSET 5u
#define KNX_FIELD_CONTROL1_PRIO_OFFSET   2u

#define KNX_FIELD_CONTROL1_RESERVED      0b00100000

#define KNX_FIELD_CONTROL2_DESTTYPE_OFFSET      7u
#define KNX_FIELD_CONTROL2_COUNTER_OFFSET       4u
#define KNX_FIELD_CONTROL2_EXTFRAME_OFFSET      0u

#define KNX_FIELD_COMMAND_COMMAND_OFFSET 6u
#define KNX_FIELD_COMMAND_DATA_OFFSET    0u

void sendKnx()
{
	char buf[16];
	/* ip header */
	set_u08(&buf[ 0u], KNX_FIELD_HEADER_LEN);      /* fixed header length */
	set_u08(&buf[ 1u], KNX_FIELD_HEADER_VER);      /* fixed protocol version */
	set_u16(&buf[ 2u], KNX_SERVICE_ROUTING_IND);   /* service type */
	set_u16(&buf[ 4u], KNX_FIELD_HEADER_LEN + 0u); /* total length */

	/* body */
	/* body - connection */
	set_u08(&buf[ 6u], 0x29u);                  /* message code */
	set_u08(&buf[ 7u], 0x00u);                  /* body length */

	/* body - cEMI */
	set_u08(&buf[ 8u], KNX_FIELD_CONTROL1_RESERVED
	                 | (KNX_FRAME_FORMAT_STD           << KNX_FIELD_CONTROL1_FORMAT_OFFSET)
	                 | (1u                             << KNX_FIELD_CONTROL1_REPEAT_OFFSET)
	                 | (KNX_PRIORITY_NORMAL            << KNX_FIELD_CONTROL1_PRIO_OFFSET));                  /* control field */
	set_u08(&buf[ 9u], (1u                             << KNX_FIELD_CONTROL2_DESTTYPE_OFFSET)
	                 | (5u                             << KNX_FIELD_CONTROL2_COUNTER_OFFSET)
	                 | (KNX_EXTFRAME_FORMAT_STD        << KNX_FIELD_CONTROL2_EXTFRAME_OFFSET));     /* Typ Zieladr.(1b); Rout.Zähl.(3b); Ext. Frame Format(4b). */
	set_u16(&buf[10u], 0x1104u);                /* source address */
	set_u16(&buf[12u], 0x0001u);                /* target address */
	set_u08(&buf[13u], 0x01u);                  /* 0000 + Längenfeld(4b). */
	set_u16(&buf[14u], (KNX_COMMAND_VALUE_WRITE        << KNX_FIELD_COMMAND_COMMAND_OFFSET)
	                 | (0u                             << KNX_FIELD_COMMAND_DATA_OFFSET));                /* TPCI/APCI & data */
	g_udp.send(buf, 16);
}


void sayHello()
{
    Serial.print("Hello Sming! Let's do smart things.");
    Serial.print(" Time : ");
    Serial.println(micros());
    Serial.println();

    Serial.printf("This is Hello message %d \r\n", ++helloCounter);
    //g_udp.sendString("Helloooo!!");
    sendKnx();


    if (g_temperature.MeasureStatus()) {
        Serial.printf("Measurement %f \r\n", g_temperature.GetCelsius(0));
        g_temperature.StartMeasure();
    }
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true);

	WifiAccessPoint.enable(false);

	WifiStation.config(WLAN_SSID, WLAN_SECRET);
	Serial.println("Connecting to wifi...");
	WifiStation.waitConnection(networkOk);

	procTimer.initializeMs(2000, sayHello).start();
	g_temperature.Init(2);
	g_temperature.StartMeasure();
}
