/*
 * eth.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#include "eth.h"
#include "ipv4.h"

#if(LOGGING == 1)
#include <stdio.h>
#endif
#include <string.h>

uint8_t ETH_MAC_BROADCAST[ETH_MAC_LENGTH] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ETH_OWN_MAC[ETH_MAC_LENGTH];

uint8_t ETH_MatchMAC(uint8_t *source, const uint8_t *dest){
	for(uint8_t idx = 0; idx < ETH_MAC_LENGTH; idx++){
		if(source[idx] != dest[idx])
			return 0;
	}

	return 1;
}

void ETH_DefineMAC(uint8_t *value){
	for(uint8_t idx = 0; idx< ETH_MAC_LENGTH; idx++)
		ETH_OWN_MAC[idx] = value[idx];
}

void ETH_GetOwnMAC(uint8_t *buffer){
	for(uint8_t idx = 0; idx < ETH_MAC_LENGTH; idx++)
		buffer[idx] = ETH_OWN_MAC[idx];
}

#if(LOGGING == 1)
void ETH_PrintMAC(uint8_t *mac, char *buffer){
	// TODO: use no-lib approach
	sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
#endif

int ETH_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply){
	eth_header_t *eth = (eth_header_t *)msg;
	eth_header_t *ethReply = (eth_header_t *)reply;
	uint8_t *auxMACBuffer[ETH_MAC_LENGTH];
	uint16_t type = REVERT_16BITS(eth->etherType);

	memcpy(ethReply->destMAC, eth->sourceMAC, 6);
	memcpy(ethReply->sourceMAC, eth->destMAC, 6);
	ethReply->etherType = eth->etherType;

#if(LOGGING == 1)
	char sMAC[ETH_MAC_STR_LENGTH], dMAC[ETH_MAC_STR_LENGTH];
	ETH_PrintMAC(eth->sourceMAC, sMAC);
	ETH_PrintMAC(eth->destMAC, dMAC);
	printf("DYNAMICO: Received packet from MAC \'%s\' to MAC \'%s\'\n", sMAC, dMAC);
#endif

#if(DYN_CONF_ACCEPT_MAC_BROADCAST == 1)
	if((ETH_MatchMAC(eth->destMAC, ETH_MAC_BROADCAST)) || (ETH_MatchMAC(eth->destMAC, ETH_OWN_MAC))){
#else
	if(ETH_MatchMAC(eth->destMAC, ETH_OWN_MAC)){
#endif
		uint8_t *payload = (reply + PACKET_ETH_LENGTH);
		int responseLength = 0;
		msg += PACKET_ETH_LENGTH;
		length -= PACKET_ETH_LENGTH;

		if(type == ETH_TYPE_IPV4){
			// IPv4 header
			responseLength = IPV4_ProcessPacket(msg, length, eth->sourceMAC, payload);
		}else if(type == ETH_TYPE_ARP){
			responseLength = 0;	// TODO: implement ARP
		}else if(type == ETH_TYPE_WOL){
			responseLength = 0;	// TODO: implement WOL
		}else{
			return ETH_ERROR_TYPE_NOT_SUPPORTED;
		}

		if(responseLength == 0)
			return 0;
		else
			return (responseLength + PACKET_ETH_LENGTH);
	}else{
		// packet not for us!
		return ETH_ERROR_WRONG_DEST_MAC;
	}
}
