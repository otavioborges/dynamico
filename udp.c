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

	uint8_t *payload = (reply + PACKET_UDP_LENGTH);
	int responseLength = 0;
	msg += PACKET_UDP_LENGTH;
	length -= PACKET_UDP_LENGTH;

	if(udp->destPort == DHCP_SERVER_PORT){
		replyLength = DHCP_Parse(sourceMaC, (dhcp_header_t *)msg, payload, replyOnBroadcast);

		if(replyLength == 0)
			return 0;

		return (replyLength + PACKET_UDP_LENGTH);
	}

	return 0; // we don't support that protocol
}


