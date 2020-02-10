/*
 * udp.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#if(LOGGING == 1)
#include <stdlib.h>
#include <stdio.h>
#endif
#include <string.h>

#include "udp.h"
#include "dhcp.h"
#include "ntp.h"

static udp_callback_t g_callback = NULL;
static uint16_t g_callbackPort = 0;

int UDP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast){
	udp_header_t *udp;
	udp_header_t *replyUdp;
	int replyLength = 0;

	udp = (udp_header_t *)msg;

	// point the reply
	replyUdp = (udp_header_t *)reply;

	// organize fields for reply
	replyUdp->sourcePort = udp->destPort;
	replyUdp->destPort = udp->sourcePort;
	replyUdp->checksum = 0;
	replyUdp->length = udp->length;

	uint8_t *payload = (reply + PACKET_UDP_LENGTH);
	int responseLength = 0;
	msg += PACKET_UDP_LENGTH;
	length -= PACKET_UDP_LENGTH;

#if (DYN_CONF_DHCP_CLIENT == 1)
	if((udp->destPort == DHCP_SERVER_PORT) || (udp->destPort == DHCP_CLIENT_PORT)){
#else
	if(udp->destPort == DHCP_SERVER_PORT){
#endif
		replyLength = DHCP_Parse(sourceMaC, (dhcp_header_t *)msg, payload, replyOnBroadcast);
	}else if(udp->destPort == REVERT_16BITS(NTP_UDP_PORT)){
		replyLength = NTP_Parse((ntp_header_t *)msg, payload);
	}else if(udp->destPort == REVERT_16BITS(g_callbackPort)){
		if(g_callback != NULL){
			replyLength = (*g_callback)(msg, length, payload);
			g_callback = NULL;
			g_callbackPort = 0;
		}else{
			return 0; // port not supported
		}
	}

	if(replyLength == 0)
		return 0;

	replyUdp->length = REVERT_16BITS((replyLength + PACKET_UDP_LENGTH));
	return (replyLength + PACKET_UDP_LENGTH);
}

int UDP_GetHeader(uint8_t *payload, uint16_t dataLength, uint16_t destPort, uint16_t srcPort){
	udp_header_t *udp = (udp_header_t *)payload;

	udp->sourcePort = REVERT_16BITS(srcPort);
	udp->destPort = REVERT_16BITS(destPort);
	udp->checksum = 0;
	udp->length = REVERT_16BITS((dataLength + PACKET_UDP_LENGTH));

	return PACKET_UDP_LENGTH;
}

int UDP_SendPacket(uint32_t destination, uint8_t *data, uint16_t srcPort, uint16_t destPort, uint32_t length, uint8_t *payload, udp_callback_t callback){
	uint32_t totalLength = 0;
	uint8_t *macDest;

	macDest = IPV4_GetMAC(destination);
	totalLength = ETH_CreateHeader(payload, macDest, ETH_TYPE_IPV4);
	totalLength += IPV4_GetHeader((payload + totalLength), (length + PACKET_UDP_LENGTH), destination, IPv4_PROTOCOL_UDP);
	totalLength += UDP_GetHeader((payload + totalLength), length, destPort, srcPort);

	memcpy((payload + totalLength), data, length);

	if(callback != NULL){
		g_callbackPort = srcPort;
		g_callback = callback;
	}
	return (totalLength + length);
}
