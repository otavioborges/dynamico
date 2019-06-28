/*
 * eth.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#include <stdio.h>
#include "eth.h"

uint8_t ETH_MAC_BROADCAST[ETH_MAC_LENGTH] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ETH_OWN_MAC[ETH_MAC_LENGTH];

uint8_t ETH_MatchMAC(uint8_t *source, const uint8_t *dest){
	for(uint8_t idx = 0; idx < ETH_MAC_LENGTH; idx++){
		if(source[idx] != dest[idx])
			return 0;
	}

	return 1;
}

void ETH_PrintMAC(uint8_t *mac, char *buffer){
	// TODO: use no-lib approach
	sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void ETH_DefineMAC(uint8_t *value){
	for(uint8_t idx = 0; idx< ETH_MAC_LENGTH; idx++)
		ETH_OWN_MAC[idx] = value[idx];
}
