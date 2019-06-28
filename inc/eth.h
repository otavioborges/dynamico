/*
 * eth_header.h
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#ifndef ETH_H_
#define ETH_H_

#include <stdint.h>

#define PACKET_ETH_LENGTH	14

#define ETH_MAC_LENGTH		6
#define ETH_MAC_STR_LENGTH	18
extern uint8_t ETH_MAC_BROADCAST[];
extern uint8_t ETH_OWN_MAC[];

typedef struct{
	uint8_t destMAC[6];
	uint8_t sourceMAC[6];
	uint16_t etherType;
}eth_header_t;

uint8_t ETH_MatchMAC(uint8_t *source, const uint8_t *dest);
void ETH_PrintMAC(uint8_t *mac, char *buffer);
void ETH_DefineMAC(uint8_t *value);

#endif /* ETH_H_ */
