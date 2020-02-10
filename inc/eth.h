/*
 * eth_header.h
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#ifndef ETH_H_
#define ETH_H_

#include <stdint.h>
#include "dynamico_conf.h"

#define PACKET_ETH_LENGTH				14

#define ETH_MAC_LENGTH					6
#define ETH_MAC_STR_LENGTH				18

#define ETH_ERROR_TYPE_NOT_SUPPORTED	-1
#define ETH_ERROR_WRONG_DEST_MAC		-2

#define REVERT_16BITS(x)				(((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF))
#define REVERT_32_BITS(x)				(((x << 24) & 0xFF000000) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | ((x >> 24) & 0x000000FF))

typedef enum{
	ETH_TYPE_IPV4		= 0x0800U,
	ETH_TYPE_ARP		= 0x0806U,
	ETH_TYPE_WOL		= 0x0842U,
}eth_type_t;

typedef struct{
	uint8_t destMAC[6];
	uint8_t sourceMAC[6];
	uint16_t etherType;
}eth_header_t;

uint8_t ETH_MatchMAC(uint8_t *source, const uint8_t *dest);
void ETH_DefineMAC(uint8_t *value);
uint8_t * ETH_GetOwnMAC(void);
void ETH_GetResponseMAC(uint8_t *mac);
void ETH_PrintMAC(uint8_t *mac, char *buffer);

int ETH_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply);
int ETH_CreateHeader(uint8_t *buffer, uint8_t *destination, eth_type_t type);

#endif /* ETH_H_ */
