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

int UDP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint32_t *replyOnBroadcast){
	udp_header_t *udp;
	udp_header_t *replyUdp;

	uint8_t *payload;
	int replyLength = 0;

	udp = (udp_header_t *)msg;
	payload = (uint8_t *)(msg + PACKET_UDP_LENGTH);

	// point the reply
	replyUdp = (udp_header_t *)reply;

	// organize fields for reply

	replyUdp->sourcePort = udp->destPort;
	replyUdp->destPort = udp->sourcePort;
	replyUdp->checksum = 0;

	if(udp->destPort == DHCP_SERVER_PORT){
		uint32_t replyOnBroadcast = 0;
		replyLength = (int)DHCP_Parse(eth->sourceMAC, (dhcp_header_t *)payload,
				(reply + PACKET_ETH_LENGTH + headerLength + PACKET_UDP_LENGTH), &replyOnBroadcast);

		if(replyLength == 0)
			return 0;

		if(replyOnBroadcast == 1)
			replyIpv4->destAddr = replyOnBroadcast;
		else if(replyOnBroadcast == 0)
			replyIpv4->destAddr = ipv4->sourceAddr;
		else
			replyIpv4->destAddr = REVERT_32_BITS(replyOnBroadcast);

		replyUdp->length = REVERT_16BITS(replyLength + PACKET_UDP_LENGTH);
		replyIpv4->length = REVERT_16BITS(replyLength + PACKET_UDP_LENGTH + PACKET_IPv4_LENGTH);
		return (replyLength + PACKET_ETH_LENGTH + PACKET_UDP_LENGTH + headerLength);
	}

	return 0; // we don't support that protocol
}


