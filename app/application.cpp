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

#include <user_config.h>
#include <SmingCore.h>
#include <libraries/DS18S20/ds18s20.h>
#include "knx.h"
#include <deque>
#include <vector>
#include <algorithm>

#define KNX_GROUPADDRESS_TEMPERATURE KNX_GROUP_ADDRESS(3,1,0)

#ifndef MQTT_HOST
#define MQTT_HOST "192.168.3.117"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USER
#define MQTT_USER ""
#endif

#ifndef MQTT_PASS
#define MQTT_PASS ""
#endif


void on_mqtt_message(String topic, String message);
void on_mqtt_complete(TcpClient& client, bool flag);

Timer      g_timer;
DS18S20    g_temperature;
bool       g_network;
MqttClient g_mqtt(MQTT_HOST, MQTT_PORT, on_mqtt_message);
String     g_topic;
std::deque<float> g_values;
const std::deque<float>::size_type g_values_max = 3;


void mqtt_connect()
{
    g_mqtt.setCompleteDelegate(on_mqtt_complete);
    g_mqtt.setWill(g_topic, "UNDEF", 0, false);
    if (strlen(MQTT_USER) == 0u) {
        g_mqtt.connect("esp8266");
    } else {
        g_mqtt.connect("esp8266", MQTT_USER, MQTT_PASS);
    }
}

void send_temperature(float val)
{
    if (g_network) {
        if (std::isnan(val)) {
            g_mqtt.publish(g_topic, "UNDEF");
        } else {
            g_mqtt.publish(g_topic, String(val));
        }
        knx_send_group_write_f16(KNX_GROUPADDRESS_TEMPERATURE, val);
    }
}

void on_network_ok()
{
    Serial.println("Connected to wifi");
    Serial.print("IP: ");
    Serial.println(WifiStation.getIP().toString());

    Serial.print("GW: ");
    Serial.println(WifiStation.getNetworkGateway().toString());

    knx_init();
    g_network = true;
}

template <typename Iter>
typename std::iterator_traits<Iter>::value_type median(Iter begin, Iter end)
{
    typedef typename std::iterator_traits<Iter>::value_type value_type;
    typename std::vector<value_type> v(begin, end);
    auto l = v.size();
    auto n = l / 2;
    std::nth_element(v.begin(), v.begin()+n, v.end());
    auto m = v[n];
    if (l&1) {
        return m;
    } else {
        return (m + *std::max_element(v.begin(), v.begin()+n)) / 2;
    }
}

void on_timer_event()
{
    Serial.print(" Time : ");
    Serial.println(micros());
    Serial.println();

    if (g_mqtt.getConnectionState() != eTCS_Connected) {
        mqtt_connect();
    }

    if (g_temperature.MeasureStatus()) {
        Serial.printf("Measurement in progress \r\n");
    } else {
        if (g_values.size() > g_values_max - 1) {
            g_values.pop_front();
        }
        if (g_temperature.IsValidTemperature(0)) {
            float temparature = g_temperature.GetCelsius(0);
            Serial.printf("Measurement %f \r\n", temparature);
            g_values.push_back(temparature);
        } else {
            Serial.printf("Measurement failed \r\n");
        }
        if (g_values.empty()) {
            send_temperature(NAN);
        } else {
            send_temperature(median(g_values.begin(), g_values.end()));

        }
        g_temperature.StartMeasure();
    }

}

void on_mqtt_message(String topic, String message)
{
    Serial.printf("MQTT [%s]:%s\r\n", topic.c_str(), message.c_str());
}

void on_mqtt_complete(TcpClient& client, bool flag)
{
    Serial.printf("MQTT complete: %d\r\n", flag);
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true);

	g_topic  = "temperature/";
	g_topic  += WifiStation.getMAC();

	WifiAccessPoint.enable(false);

	WifiStation.config(WIFI_SSID, WIFI_PWD);
	Serial.println("Connecting to wifi...");
	WifiStation.waitConnection(on_network_ok);

	g_timer.initializeMs(2000, on_timer_event).start();
	g_temperature.Init(2);
	g_temperature.StartMeasure();
}
