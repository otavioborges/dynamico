/*
 * arp.c
 *
 *  Created on: 26 de dez de 2019
 *      Author: otavio
 */

#include <stdlib.h>
#include <string.h>
#include "arp.h"
#include "eth.h"
#include "ipv4.h"

static uint8_t noMac[6] = {0, 0, 0, 0, 0, 0};

int ARP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply){
	arp_header_t *arp = (arp_header_t *)msg;
	arp_header_t *replyArp = (arp_header_t *)reply;
	uint32_t responseIP;
	uint8_t *targetMAC;

	if(arp->hlen != 6)	// Only 6 bytes MAC
		return 0;

	if(arp->plen != 4)	// Only IPv4
		return 0;

	responseIP = REVERT_32_BITS((((uint32_t)arp->tpa[1] << 16) + arp->tpa[0]));
	if(arp->oper == REVERT_16BITS(ARP_OPER_REQUEST)){
		targetMAC = (uint8_t *)arp->tha;
		if((memcmp(targetMAC, noMac, 6) == 0)){
			if(IPV4_GetOwnIP() != responseIP)
				return 0;
		}else if(ETH_MatchMAC(targetMAC, ETH_GetOwnMAC()) == 0){
			return 0;
		}

		// We are the target reply
		replyArp->htype = 0x0100;
		replyArp->ptype = REVERT_16BITS(ETH_TYPE_IPV4);
		replyArp->hlen = 6;
		replyArp->plen = 4;
		replyArp->oper = REVERT_16BITS(ARP_OPER_REPLY);

		memcpy(replyArp->sha, ETH_GetOwnMAC(), 6);
		replyArp->spa[1] = REVERT_16BITS(((uint16_t)IPV4_GetOwnIP()));
		replyArp->spa[0] = REVERT_16BITS(((uint16_t)(IPV4_GetOwnIP() >> 16)));

		memcpy(replyArp->tha, arp->sha, 6);
		memcpy(replyArp->tpa, arp->spa, 4);

		return PACKET_ARP_LENGTH;
	}else{
		// get the MAC from the replier
		targetMAC = (uint8_t *)arp->tha;
		if(ETH_MatchMAC(targetMAC, ETH_GetOwnMAC())){
			responseIP = ((uint32_t)arp->spa[1] << 16) + arp->spa[0];
			IPV4_AddHost((uint8_t *)arp->sha, REVERT_32_BITS(responseIP));
		}

		return 0;
	}
}

int ARP_CreateRequest(uint32_t ip, uint8_t *mac, uint8_t *payload){
	uint8_t brdcst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	arp_header_t *arp = (arp_header_t *)(payload + PACKET_ETH_LENGTH);

	ETH_CreateHeader(payload, brdcst, ETH_TYPE_ARP);

	arp->htype = 0x0100;
	arp->ptype = REVERT_16BITS(ETH_TYPE_IPV4);
	arp->hlen = 6;
	arp->plen = 4;
	arp->oper = REVERT_16BITS(ARP_OPER_REQUEST);

	memcpy(arp->sha, ETH_GetOwnMAC(), 6);
	arp->spa[1] = REVERT_16BITS(((uint16_t)IPV4_GetOwnIP()));
	arp->spa[0] = REVERT_16BITS(((uint16_t)(IPV4_GetOwnIP() >> 16)));

	if(mac != NULL)
		memcpy(arp->tha, mac, 6);
	else
		memset(arp->tha, 0, 6);

	arp->tpa[1] = REVERT_16BITS((uint16_t)ip);
	arp->tpa[0] = REVERT_16BITS(((uint16_t)(ip >> 16)));
	return (PACKET_ARP_LENGTH + PACKET_ETH_LENGTH);
}
