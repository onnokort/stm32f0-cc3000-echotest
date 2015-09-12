/* (C)opright 2013 Onno Kortmann <onno@gmx.net> 
 * License: GPLv3, see COPYING
 * This is a heavily changed adaption of the basic_wifi_application.c from TI,
 * portions copyright as follows: */

/*****************************************************************************
*
*  basic_wifi_application.c - CC3000 Slim Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/f0/gpio.h>
#include <cc3000/wlan.h>
#include <cc3000/evnt_handler.h>
#include <cc3000/nvmem.h>
#include <cc3000/socket.h>
#include <cc3000/cc3000_common.h>
#include <cc3000/netapp.h>
#include <cc3000/spi.h>
#include <cc3000/hci.h>
#include <stdio.h>

#include "lowlevel.h"
#include "wlan_keys.h"
#include "stlinky.h"
#include <cc3000/hardware.h>

static char wlan_connected, wlan_dhcp;
static uint8_t dhcp_addr[4];

#define DEBUG       0

#if DEBUG
#define DBGP(args...) printf(args)
#else
#define DBGP(args...)
#endif

void CC3000_callback(long lEventType, char *data, unsigned char length) {
    length=length;
	if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT) {
		// Turn on the LED1
		led(0, 1);
        wlan_connected=1;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT) {		
		// Turn off the LED1
		led(0, 0);                                     
	}

	if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP) {
        led(1, 1);
		if (data[20]==0) { //NETAPP_IPCONFIG_MAC_OFFSET == 20: FIXME, hardcoded!
            dhcp_addr[0]=data[0];
            dhcp_addr[1]=data[1];
            dhcp_addr[2]=data[2];
            dhcp_addr[3]=data[3];
            wlan_dhcp=1;
        }
    }
}

int main() {
    int res;
	// Init hardware
	ll_init();
    cc3k_hardware_init();

#if DEBUG    
    // initialize stlinky pseudo UART (see FIXME)
	stlinky_init();
#endif
    
    DBGP("stlinky initialized.\n");
    
    ll_enable_interrupts();
    DBGP("Interrupts enabled.\n");
    
    // The callbacks here are declared in hardware.h. They
    // reroute everything through the functions that are now
    // in board.c.
	wlan_init(CC3000_callback, NULL, NULL, 
              NULL, WlanReadInterruptPin, WlanInterruptEnable, 
              WlanInterruptDisable, WriteWlanPin);
    DBGP("WLAN initialized.\n");

    int i;
    
	wlan_start(0);	
	DBGP("WLAN started.\n");
    
	// Mask out all non-required events from CC3000
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT
                        |HCI_EVNT_WLAN_ASYNC_PING_REPORT);
    DBGP("Set WLAN event mask.\n");
	cc3k_delay_ms(1000); 

	unsigned char buf[MAC_ADDR_LEN];
    
    if (nvmem_read_sp_version(buf)==0) {
        DBGP("Firmware version: %d.%d.\n", buf[0], buf[1]);
    } else {
        DBGP("Error while reading firmware version.\n");
        panic();
    }

    if (nvmem_get_mac_address(buf)==0) {
        DBGP("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } else {
        DBGP("Error while reading MAC address.\n");
        panic();
    }

    wlan_ioctl_set_connection_policy(0, 0, 0);
    wlan_ioctl_del_profile(255);
    DBGP("Disabled/removed old connections.\n"); // FIXME: necessary?
    
    DBGP("Connecting...\n");

    res=wlan_connect(WLAN_SEC_WPA2,
                         WLAN_SSID,
                         strlen(WLAN_SSID),
                         NULL,
                         (unsigned char *)WLAN_KEY,
                         strlen(WLAN_KEY));
    DBGP("Connect status: %d\n", res);
    if (res) {
        DBGP("Error while connecting.\n");
        panic();
    }

    while(!wlan_connected || !wlan_dhcp) {
        DBGP("Waiting for connection to WLAN...\n");
        cc3k_delay_ms(500);
    }
    DBGP("Connected and DHCP finished.\n");
    DBGP("IP: %d.%d.%d.%d\n", 
         dhcp_addr[3],
         dhcp_addr[2],
         dhcp_addr[1],
         dhcp_addr[0]);

    // advertise using a bunch of mDNS packets
    // use 'ZeroConf Browser' on Android, or tcpdump or
    // whatever to see.
    static char *device_name="STM32F0DISCOVERY-CC3K";
    
    for (i=0; i<3; i++)
        mdnsAdvertiser(1,device_name,strlen(device_name));
    DBGP("Advertised on zeroconf.\n");
    
    sockaddr addr;
    socklen_t sa_size=sizeof(addr);
    memset(addr.sa_data, 0, sizeof(addr.sa_data));
    // set port
    addr.sa_family=AF_INET;
    addr.sa_data[0]=0;
    addr.sa_data[1]=23; // telnet

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DBGP("Created socket: %d\n", s);

    
    res=bind(s, &addr, sa_size);
    DBGP("Bound: %d\n", s);
    
    res=listen(s, 1);
    DBGP("Listen: %d\n", s);

    sockaddr client;

    while(1) {
        while(1) {
            res=accept(s, &client, &sa_size);
            DBGP("Accept status: %d\n", res);
            if (res>0)
                break;
        }
    
        // client socket handle
        int csock=res;
        char trxbuf[4];

        while (1) {
            res=recv(csock, trxbuf, 4, 0);
            DBGP("Received: %d\n", res);
            if (res<0) {
                closesocket(res);
                break;
            }
            res=send(csock, trxbuf,  res, 0);
            DBGP("Sent: %d\n", res);
            if (res<0) {
                closesocket(res);
                break;
            }
        }
    }
}
