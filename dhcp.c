/*
 * dhcp.c
 *
 *  Created on: Jun 17, 2019
 *      Author: root
 */

#include <string.h>
#include "dhcp.h"
#include "eth.h"
#include "ipv4.h"
#include "udp.h"

#define DHCP_RANGE_BUFFER_LENGTH	128
#define DHCP_MAGIC_COOKIE			0x63538263

uint32_t g_serverAddr = 0x0a0a0001;
static uint32_t g_network = 0x0a0a0000;
static uint32_t g_netmask = 0xffffff00;
static uint32_t g_nextIP = 0x0a;
static uint32_t g_initialRange = 0x0a;
static uint32_t g_endRange = 0xff;

static dhcp_lease_t g_leases[DHCP_RANGE_BUFFER_LENGTH];
static uint32_t g_currentLeases = 0;

static char g_domainName[] = "oleivas.com.br";
static uint8_t ETH_MAC_BROADCAST[ETH_MAC_LENGTH] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static dhcp_callback_t g_callback = NULL;

static uint32_t DHCP_Lease(uint8_t *mac, char *name, uint8_t allowAlocate){
	for(uint32_t idx = 0; idx < g_currentLeases; idx++){
		if(ETH_MatchMAC(mac, g_leases[idx].hwAddr))
			return idx;
	}

	// Received a Request if DHCP is not on table just return a error
	if(allowAlocate == 0)
		return 0xFFFFFFFF;

	// could find lease create new!
	memcpy(g_leases[g_currentLeases].hwAddr, mac, 6);
	strcpy(g_leases[g_currentLeases].name, name);
	g_leases[g_currentLeases].issueTime = 0;
	g_leases[g_currentLeases].ipAddr = g_network + g_nextIP;
	g_nextIP++;
	g_currentLeases++;

	return (g_currentLeases-1);
}

static uint16_t DHCP_request(dhcp_header_t *msg, uint8_t *reply, uint32_t * replyAddr){
	uint32_t transactionID;
	dhcp_option_t *option;
	dhcp_lease_report_t report;
	dhcp_op_t op = DHCP_OP_INVALID;
	char clientName[64];
	clientName[0] = '\0';
	uint32_t leaseIdx = 0;

	if(msg->hType != ADDR_TYPE_ETHERNET)
		return 0;	// we only know how to deal with IEEE 802

	if(msg->hLen != 6)
		return 0;	// only IEEE MAC

	transactionID = msg->XID;
	if(*((uint32_t *)msg->options) != DHCP_MAGIC_COOKIE)
		return 0;

	// run Options and define some variables
	option = (dhcp_option_t *)(msg->options + 4);
	while(option->id != 0xFF){
		if(option->id == 0x35){
			op = (dhcp_op_t)option->value;
		}else if(option->id == 0x0C){
			memcpy(clientName, &option->value, option->length);
			clientName[option->length] = '\0';
		}

		option = (dhcp_option_t *)((&option->value) + option->length);
	}

	if(op == 0xFF)	// TRETA!
		return 0;

	// assemble response
	dhcp_header_t *replyHeader = (dhcp_header_t *)reply;

	replyHeader->op = 2;
	replyHeader->hType = ADDR_TYPE_ETHERNET;
	replyHeader->hLen = 6;
	replyHeader->hops = 0;
	replyHeader->XID = transactionID;
	replyHeader->secs = 0;
	replyHeader->flags = msg->flags;
	replyHeader->cIAddr = 0;
	replyHeader->sIAddr = 0;
	replyHeader->gIAddr = 0;
	memset(replyHeader->cHAddr, 0, 16);
	memcpy(replyHeader->cHAddr, msg->cHAddr, 6);

	uint32_t optionOffset = 4;
	*(uint32_t *)(replyHeader->options) = DHCP_MAGIC_COOKIE;

	// If Discover send offer!
	if(op == DHCP_OP_DISCOVERY){
		leaseIdx = DHCP_Lease(msg->cHAddr, clientName, 1);

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 53;
		option->length = 1;
		option->value = DHCP_OP_OFFER;
		optionOffset += 3;
	}else if(op == DHCP_OP_REQUEST){ // send ACK/NACK
		leaseIdx = DHCP_Lease(msg->cHAddr, clientName, 0);

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 53;
		option->length = 1;
		if(leaseIdx == 0xFFFFFFFF)
			option->value = DHCP_OP_NAC;
		else
			option->value = DHCP_OP_ACK;
		optionOffset += 3;
	}

	if(leaseIdx != 0xFFFFFFFF)
		replyHeader->yIAddr = REVERT_32_BITS(g_leases[leaseIdx].ipAddr);
	else
		replyHeader->yIAddr = 0;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 54;
	option->length = 4;
	*((uint32_t *)(&option->value)) = REVERT_32_BITS(g_serverAddr);
	optionOffset += 6;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 51;
	option->length = 4;
	*((uint32_t *)(&option->value)) = REVERT_32_BITS(86400U); // 1day
	optionOffset += 6;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 1;
	option->length = 4;
	*((uint32_t *)(&option->value)) = REVERT_32_BITS(g_netmask);
	optionOffset += 6;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 3;
	option->length = 4;
	*((uint32_t *)(&option->value)) = REVERT_32_BITS(g_serverAddr);
	optionOffset += 6;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 15;
	option->length = 14;
	memcpy(((uint8_t *)(&option->value)), g_domainName, 14);
	optionOffset += 14 + 2;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 12;
	option->length = strlen(clientName);
	memcpy(((uint8_t *)(&option->value)), clientName, option->length);
	optionOffset += option->length + 2;

	option = (dhcp_option_t *)(replyHeader->options + optionOffset);
	option->id = 0xFF; // EOP
	optionOffset++;

	report.ip_lease = g_leases[leaseIdx].ipAddr;
	memcpy(report.mac, msg->cHAddr, 6);
	report.name = g_leases[leaseIdx].name;
	if(g_callback != NULL)
		g_callback(report);

	if((*replyAddr == 0) && (leaseIdx != 0xFFFFFFFF))
		*replyAddr = g_leases[leaseIdx].ipAddr;

	return (DHCP_OVERHEAD + optionOffset);
}

static uint16_t DHCP_reply(dhcp_header_t *msg, uint8_t *reply, uint32_t *replyAddr){
	uint32_t transactionID;
	dhcp_option_t *option;
	dhcp_lease_report_t report;
	dhcp_op_t op = DHCP_OP_INVALID;
	char clientName[64];
	clientName[0] = '\0';
	uint32_t leaseIdx = 0;
	uint32_t netmask = 0;
	uint32_t gateway = 0;

	if(msg->hType != ADDR_TYPE_ETHERNET)
		return 0;	// we only know how to deal with IEEE 802

	if(msg->hLen != 6)
		return 0;	// only IEEE MAC

	transactionID = msg->XID;
	if(*((uint32_t *)msg->options) != DHCP_MAGIC_COOKIE)
		return 0;

	// run Options and define some variables
	option = (dhcp_option_t *)(msg->options + 4);
	while(option->id != 0xFF){
		if(option->id == 0x35){
			op = (dhcp_op_t)option->value;
		}else if(option->id == 0x0C){
			memcpy(clientName, &option->value, option->length);
			clientName[option->length] = '\0';
		}else if(option->id == 1){
			netmask = *((uint32_t *)&option->value);
		}else if(option->id == 3){
			gateway = *((uint32_t *)&option->value);
		}

		option = (dhcp_option_t *)((&option->value) + option->length);
	}

	if(op == 0xFF)	// TRETA!
		return 0;

	// assemble response
	dhcp_header_t *replyHeader = (dhcp_header_t *)reply;

	replyHeader->op = 1;
	replyHeader->hType = ADDR_TYPE_ETHERNET;
	replyHeader->hLen = 6;
	replyHeader->hops = 0;
	replyHeader->XID = transactionID;
	replyHeader->secs = 0;
	replyHeader->flags = msg->flags;
	replyHeader->cIAddr = 0;
	replyHeader->sIAddr = msg->sIAddr;
	replyHeader->gIAddr = 0;
	memset(replyHeader->cHAddr, 0, 16);
	memcpy(replyHeader->cHAddr, msg->cHAddr, 6);

	uint32_t optionOffset = 4;
	*(uint32_t *)(replyHeader->options) = DHCP_MAGIC_COOKIE;

	// If Offer send Request!
	if(op == DHCP_OP_OFFER){
		IPV4_DefineIP(REVERT_32_BITS(msg->yIAddr), REVERT_32_BITS(netmask), REVERT_32_BITS(gateway));
		leaseIdx = DHCP_Lease(msg->cHAddr, clientName, 1);

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 53;
		option->length = 1;
		option->value = DHCP_OP_REQUEST;
		optionOffset += 3;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 50;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(msg->yIAddr);
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 54;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(msg->sIAddr);
		optionOffset += 6;
	}else if(op == DHCP_OP_ACK){ // send ACK/NACK
		return 0; // nothing to do for now!
	}

	return (DHCP_OVERHEAD + optionOffset);
}

void DHCP_Init(uint32_t network, uint32_t netmask, uint32_t initialRange, uint32_t endRange, uint32_t server){
	g_network = network;
	g_netmask = netmask;
	g_initialRange = initialRange;
	g_endRange = endRange;
	g_serverAddr = server;
	g_nextIP = g_initialRange;
}

void DHCP_SetCallback(dhcp_callback_t callback){
	g_callback = callback;
}

int DHCP_Parse(uint8_t *clientMAC, dhcp_header_t *msg, uint8_t *reply, uint32_t *broadcast){
	uint32_t replyAddr = 0;

	if(msg->flags & 0x8000) // reply on bradcast
		*broadcast = 1;
	else
		*broadcast = 0;

	// treat the request
#if (DYN_CONF_DHCP_CLIENT == 1)
	if(msg->op == 0x01){
		return 0;
	}else if(msg->op == 0x02){
		return DHCP_reply(msg, reply, broadcast);
	}
#else
	if(msg->op == 0x01){
		return DHCP_request(msg, reply, broadcast);
	}else if(msg->op == 0x02){
		return DHCP_reply(msg, reply, broadcast);
	}
#endif
	// unknown message
	return 0;
}

void DHCP_RequestIP(uint8_t *resultMsg, uint32_t *length){
	dhcp_option_t *option;

	ipv4_header_t *ipv4Header	= (ipv4_header_t *)(resultMsg + PACKET_ETH_LENGTH);
	udp_header_t *udpHeader		= (udp_header_t *)(resultMsg + PACKET_ETH_LENGTH + PACKET_IPv4_LENGTH);
	dhcp_header_t *dhcpHeader	= (dhcp_header_t *)(resultMsg + PACKET_ETH_LENGTH + PACKET_IPv4_LENGTH + PACKET_UDP_LENGTH);

	// ETH
	ETH_CreateHeader(resultMsg, ETH_MAC_BROADCAST, ETH_TYPE_IPV4);

	// IPv4
	ipv4Header->versionIHL = 0x45;
	ipv4Header->type = 0x10;
	ipv4Header->identification = 0;
	ipv4Header->flagsOffset = 0;
	ipv4Header->timeToLive = 64;
	ipv4Header->protocol = IPv4_PROTOCOL_UDP;
	ipv4Header->checksum = 0;
	ipv4Header->sourceAddr = 0;
	ipv4Header->destAddr = 0xFFFFFFFF;

	// UDP
	udpHeader->sourcePort = REVERT_16BITS(68);
	udpHeader->destPort = REVERT_16BITS(67);
	udpHeader->checksum = 0;
	udpHeader->length = 0;

	dhcpHeader->op = DHCP_OP_DISCOVERY;
	dhcpHeader->hType = ADDR_TYPE_ETHERNET;
	dhcpHeader->hLen = 6;
	dhcpHeader->hops = 0;
	dhcpHeader->XID = 394763;
	dhcpHeader->secs = 0xFFFF;
	dhcpHeader->flags = 0;
	dhcpHeader->cIAddr = 0;
	dhcpHeader->yIAddr = 0;
	dhcpHeader->sIAddr = 0;
	dhcpHeader->gIAddr = 0;
	memset(dhcpHeader->cHAddr, 0, 16);
	memcpy(dhcpHeader->cHAddr, ETH_GetOwnMAC(), 6);

	uint32_t optionOffset = 4;
	*(uint32_t *)(dhcpHeader->options) = DHCP_MAGIC_COOKIE;

	option = (dhcp_option_t *)(dhcpHeader->options + optionOffset);
	option->id = 53;
	option->length = 1;
	option->value = DHCP_OP_DISCOVERY;
	optionOffset += 3;

	option = (dhcp_option_t *)(dhcpHeader->options + optionOffset);
	option->id = 55;
	option->length = 4;
	(&option->value)[0] = 1;
	(&option->value)[1] = 3;
	(&option->value)[2] = 15;
	(&option->value)[3] = 6;
	optionOffset += 6;

	option = (dhcp_option_t *)(dhcpHeader->options + optionOffset);
	option->id = 0xFF; // EOP
	optionOffset++;

	udpHeader->length = REVERT_16BITS((PACKET_UDP_LENGTH + DHCP_OVERHEAD + optionOffset));
	ipv4Header->length = REVERT_16BITS((PACKET_IPv4_LENGTH + PACKET_UDP_LENGTH + DHCP_OVERHEAD + optionOffset));

	*length = (PACKET_ETH_LENGTH + PACKET_IPv4_LENGTH + PACKET_UDP_LENGTH + DHCP_OVERHEAD + optionOffset);
}
