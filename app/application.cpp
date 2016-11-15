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

Timer procTimer;
int helloCounter = 0;

DS18S20            g_temperature;

#define KNX_GROUPADDRESS_TEMPERATURE KNX_GROUP_ADDRESS(3,1,0)

void networkOk()
{
    Serial.println("Connected to wifi");
    Serial.print("IP: ");
    Serial.println(WifiStation.getIP().toString());

    Serial.print("GW: ");
    Serial.println(WifiStation.getNetworkGateway().toString());

    knx_init();
}


void sayHello()
{
    Serial.print(" Time : ");
    Serial.println(micros());
    Serial.println();

    float temparature;
    if (g_temperature.MeasureStatus()) {
        temparature = g_temperature.GetCelsius(0);
        Serial.printf("Measurement %f \r\n", temparature);
        g_temperature.StartMeasure();
    } else {
        temparature = NAN;
        Serial.printf("Measurement failed \r\n");
    }

    knx_send_group_write_f16(KNX_GROUPADDRESS_TEMPERATURE, temparature);
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
