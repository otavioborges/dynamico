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

#include "tcp.h"
#include "dhcp.h"

int TCP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast){
//	udp_header_t *udp;
//	udp_header_t *replyUdp;
//	int replyLength = 0;
//
//	udp = (udp_header_t *)msg;
//
//	// point the reply
//	replyUdp = (udp_header_t *)reply;
//
//	// organize fields for reply
//	replyUdp->sourcePort = udp->destPort;
//	replyUdp->destPort = udp->sourcePort;
//	replyUdp->checksum = 0;
//	replyUdp->length = udp->length;
//
//	uint8_t *payload = (reply + PACKET_UDP_LENGTH);
//	int responseLength = 0;
//	msg += PACKET_UDP_LENGTH;
//	length -= PACKET_UDP_LENGTH;
//
//#if (DYN_CONF_DHCP_CLIENT == 1)
//	if((udp->destPort == DHCP_SERVER_PORT) || (udp->destPort == DHCP_CLIENT_PORT)){
//#else
//	if(udp->destPort == DHCP_SERVER_PORT){
//#endif
//		replyLength = DHCP_Parse(sourceMaC, (dhcp_header_t *)msg, payload, replyOnBroadcast);
//
//		if(replyLength == 0)
//			return 0;
//
//		replyUdp->length = REVERT_16BITS((replyLength + PACKET_UDP_LENGTH));
//		return (replyLength + PACKET_UDP_LENGTH);
//	}

	return 0; // we don't support that protocol
}

int TCP_SendOurData(uint8_t *reply, uint8_t *msg, uint32_t length){
//	eth_header_t *eth		= (eth_header_t *)reply;
//	ipv4_header_t *ipv4		= (ipv4_header_t *)(reply + PACKET_ETH_LENGTH);
//	udp_header_t *udp		= (udp_header_t *)(reply + PACKET_ETH_LENGTH + PACKET_IPv4_LENGTH);
//	uint8_t mac[6];
//
//	memcpy(ethReply->destMAC, eth->sourceMAC, 6);
//	memcpy(ethReply->sourceMAC, ETH_OWN_MAC, 6);
//	ethReply->etherType = eth->etherType;
}
