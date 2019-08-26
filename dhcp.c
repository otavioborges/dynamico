/*
 * dhcp.c
 *
 *  Created on: Jun 17, 2019
 *      Author: root
 */

#include <string.h>
#include "dhcp.h"
#include "eth.h"

#define DHCP_RANGE_BUFFER_LENGTH	128
#define DHCP_MAGIC_COOKIE			0x63538263

uint32_t g_serverAddr = 0x0a0a0001;
static uint32_t g_network = 0x0a0a0000;
static uint32_t g_netmask = 0xffffff00;
static uint32_t g_usedRange[DHCP_RANGE_BUFFER_LENGTH];
static uint32_t g_initialRange = 0x0a;
static uint32_t g_endRange = 0xff;

static dhcp_lease_t g_leases[DHCP_RANGE_BUFFER_LENGTH];
static uint32_t g_currentLeases = 0;

static dhcp_callback_t g_callback = NULL;

static uint16_t DHCP_request(dhcp_header_t *msg, uint8_t *reply, uint32_t * replyAddr){
	uint32_t transactionID;
	dhcp_option_t *option;
	dhcp_lease_report_t report;
	char clientName[64];

	if(msg->hType != ADDR_TYPE_ETHERNET)
		return 0;	// we only know how to deal with IEEE 802

	if(msg->hLen != 6)
		return 0;	// only IEEE MAC

	transactionID = msg->XID;

	uint32_t *payload = (uint32_t *)msg->options;

	if(*payload != DHCP_MAGIC_COOKIE)
		return 0;

	option = (dhcp_option_t *)(msg->options + 4);
	while(option->id != 0x0C){
		option = (dhcp_option_t *)((&option->value) + option->length);
		if(option->id == 0xFF)
			break;
	}

	// save clients name
	if(option->id == 0x0C){
		memcpy(clientName, &option->value, option->length);
		clientName[option->length] = '\0';
	}else{
		clientName[0] = '\0';
	}

	// request or discovery?
	option = (dhcp_option_t *)(msg->options + 4);
	while(option->id != 0x35){
		option = (dhcp_option_t *)((&option->value) + option->length);
		if(option->id == 0xFF)
			break;
	}

	if(option->id == 0xFF)
		return 0;

	if(option->value == DHCP_OP_REQUEST){ // request
		option = (dhcp_option_t *)(msg->options + 4);
		while(option->id != 0x32)
			option = (dhcp_option_t *)((&option->value) + option->length);

		uint32_t desiredIP = *((uint32_t *)&option->value);
		uint8_t macAddr[6];
		uint8_t ack = 0;

		desiredIP = REVERT_32_BITS(desiredIP);
		memcpy(macAddr, msg->cHAddr, 6);
		for(uint16_t leaseIdx = 0; leaseIdx < g_currentLeases; leaseIdx++){
			if(ETH_MatchMAC(msg->cHAddr, g_leases[leaseIdx].hwAddr)){
				if(g_leases[leaseIdx].ipAddr == desiredIP){
					ack = 1;
					break;
				}
			}
		}

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
		replyHeader->yIAddr = REVERT_32_BITS(desiredIP);
		replyHeader->sIAddr = 0;
		replyHeader->gIAddr = 0;
		memset(replyHeader->cHAddr, 0, 16);
		memcpy(replyHeader->cHAddr, msg->cHAddr, 6);

		uint32_t optionOffset = 4;
		*(uint32_t *)(replyHeader->options) = DHCP_MAGIC_COOKIE;
		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 53;
		option->length = 1;
		if(ack == 0){
			option->value = 6;
			optionOffset += 3;

			*replyAddr = desiredIP;
			return (DHCP_OVERHEAD + optionOffset);
		}else{
			option->value = 5;
			optionOffset += 3;
		}

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 54;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(g_serverAddr);
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
		option->id = 51;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(86400U); // 1day
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 58;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(7200U); // 2hours
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 58;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(72000U); // 20hours
		optionOffset += 6;


		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 6;
		option->length = 4;
		*((uint32_t *)(&option->value)) = 0x08080808;
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 0xFF; // EOP
		optionOffset++;

		*replyAddr = desiredIP;
		return (DHCP_OVERHEAD + optionOffset);
	}else if(option->value == DHCP_OP_DISCOVERY){ // discover
		uint32_t probableIP = 0;
		uint32_t lease = 0;
		for(uint16_t leaseIdx = 0; leaseIdx < g_currentLeases; leaseIdx++){
			if(ETH_MatchMAC(msg->cHAddr, g_leases[leaseIdx].hwAddr)){
				probableIP = g_leases[leaseIdx].ipAddr;
				lease = leaseIdx;
				break;
			}
		}

		if(probableIP == 0){
		// new lease
			probableIP = g_initialRange;
			for(uint16_t usedIdx = 0; usedIdx < g_currentLeases; usedIdx++){
				if(probableIP == g_usedRange[usedIdx])
					probableIP++;
			}

			g_usedRange[g_currentLeases] = probableIP;
			g_leases[g_currentLeases].ipAddr = g_network + probableIP;
			g_leases[g_currentLeases].issueTime = 0;
			memcpy(g_leases[g_currentLeases].hwAddr, msg->cHAddr, 6);
			strcpy(g_leases[g_currentLeases].name, clientName);
			lease = g_currentLeases;

			report.ip_lease = g_leases[lease].ipAddr;
			memcpy(report.mac, msg->cHAddr, 6);
			report.name = g_leases[lease].name;
			if(g_callback != NULL)
				g_callback(report);

			g_currentLeases++;
		}

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
		replyHeader->yIAddr = REVERT_32_BITS(g_leases[lease].ipAddr);
		replyHeader->sIAddr = 0;
		replyHeader->gIAddr = 0;
		memset(replyHeader->cHAddr, 0, 16);
		memcpy(replyHeader->cHAddr, msg->cHAddr, 6);

		uint32_t optionOffset = 4;
		*(uint32_t *)(replyHeader->options) = DHCP_MAGIC_COOKIE;
		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 53;
		option->length = 1;
		option->value = 2;
		optionOffset += 3;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 54;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(g_serverAddr);
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
		option->id = 51;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(86400U); // 1day
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 58;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(7200U); // 2hours
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 58;
		option->length = 4;
		*((uint32_t *)(&option->value)) = REVERT_32_BITS(72000U); // 20hours
		optionOffset += 6;


		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 6;
		option->length = 4;
		*((uint32_t *)(&option->value)) = 0x08080808;
		optionOffset += 6;

		option = (dhcp_option_t *)(replyHeader->options + optionOffset);
		option->id = 0xFF; // EOP
		optionOffset++;

		if(*replyAddr == 0)
			*replyAddr = g_leases[lease].ipAddr;

		return (DHCP_OVERHEAD + optionOffset);
	}

	return 0;
}

static uint16_t DHCP_reply(dhcp_header_t *msg, uint8_t *reply, uint32_t *replyAddr){
	// TODO: stub
	return 0;
}

void DHCP_Init(uint32_t network, uint32_t netmask, uint32_t initialRange, uint32_t endRange, uint32_t server){
	g_network = network;
	g_netmask = netmask;
	g_initialRange = initialRange;
	g_endRange = endRange;
	g_serverAddr = server;

	memset(g_usedRange, 0xFFFFFFFF, (sizeof(uint32_t) * DHCP_RANGE_BUFFER_LENGTH));
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
	if(msg->op == 0x01){
		return DHCP_request(msg, reply, broadcast);
	}else if(msg->op == 0x02){
		return DHCP_reply(msg, reply, broadcast);
	}

	// unknown message
	return 0;
}
