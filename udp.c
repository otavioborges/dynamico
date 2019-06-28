/*
 * udp.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "udp.h"

#include "eth.h"
#include "ipv4.h"
#include "dhcp.h"

#define REVERT_16BITS(x)	(((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF))

int UDP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply){
	eth_header_t *eth;
	ipv4_header_t *ipv4;
	udp_header_t *udp;

	eth_header_t *replyEth;
	ipv4_header_t *replyIpv4;
	udp_header_t *replyUdp;

	uint8_t *payload;
	int replyLength = 0;

	uint8_t headerLength = 0;

	eth = (eth_header_t *)msg;
	if(eth->etherType != 0x0008)
		return -1;

	char sMAC[ETH_MAC_STR_LENGTH], dMAC[ETH_MAC_STR_LENGTH];
	ETH_PrintMAC(eth->sourceMAC, sMAC);
	ETH_PrintMAC(eth->destMAC, dMAC);
	printf("Source MAC: %s, Dest. MAC: %s\n", sMAC, dMAC);

	uint8_t macoso[6];
	memset(macoso, 0xFF, 6);
	// TODO: accept our own MAC
	if(ETH_MatchMAC(eth->destMAC, macoso) == 0)
		return -2;

	ipv4 = (ipv4_header_t *)(msg + PACKET_ETH_LENGTH);
	if((ipv4->versionIHL >> 4) != 4)
		return -3;	// TODO: support ipv6

	if(ipv4->protocol != IPv4_PROTOCOL_UDP)
		return -4;	// TODO: support others protocols

	// TODO: accept our own IP
	if(ipv4->destAddr != IPV4_IP_BROADCAST)
		return -5;

	headerLength = (ipv4->versionIHL & 0x0F) * 4;
	udp = (udp_header_t *)(msg + PACKET_ETH_LENGTH + headerLength);
	payload = (uint8_t *)(msg + PACKET_ETH_LENGTH + headerLength + PACKET_UDP_LENGTH);

	// point the reply
	replyEth = (eth_header_t *)reply;
	replyIpv4 = (ipv4_header_t *)(reply + PACKET_ETH_LENGTH);
	replyUdp = (udp_header_t *)(reply + PACKET_ETH_LENGTH + headerLength);

	// organize fields for reply
	uint8_t maczonio[] = {0x28, 0xf8, 0xa8, 0x12, 0xd1, 0x71};
	memcpy(replyEth->destMAC, eth->sourceMAC, 6);
	memcpy(replyEth->sourceMAC, maczonio, 6);
	replyEth->etherType = eth->etherType;

	replyIpv4->versionIHL = ipv4->versionIHL;
	replyIpv4->type = ipv4->type;
	replyIpv4->identification = ipv4->identification;
	replyIpv4->flagsOffset = 0;
	replyIpv4->timeToLive = ipv4->timeToLive;
	replyIpv4->protocol = IPv4_PROTOCOL_UDP;
	replyIpv4->checksum = 0;
	replyIpv4->sourceAddr = REVERT_32_BITS(g_serverAddr);

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


