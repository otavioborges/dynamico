/*
 * ipv4_header.h
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#ifndef IPV4_H_
#define IPV4_H_

#include <stdint.h>
#include "eth.h"

#define PACKET_IPv4_LENGTH				20

#define IPV4_IP_BROADCAST				0xFFFFFFFFU
#define IPV4_IP_ALL						0

#define ETH_ERROR_WRONG_IP_VERSION		-3
#define ETH_ERROR_WRONG_DEST_IP			-4
#define ETH_ERROR_PROT_NOT_SUPPORTED	-5

#define IPV4_REPLY_TO_SENDER			0
#define IPV4_REPLY_ON_BROADCAST_ALL		1
#define IPV4_REPLY_ON_BROADCAST_SUBNET	1

extern uint32_t IPV4_OWN_IP;

typedef enum{
	IPv4_PROTOCOL_ICMP	= 1,
	IPv4_PROTOCOL_IGMP	= 2,
	IPv4_PROTOCOL_TCP	= 6,
	IPv4_PROTOCOL_UDP	= 17
}ipv4_protocol_t;

typedef struct{
	uint8_t versionIHL;
	uint8_t type;
	uint16_t length;
	uint16_t identification;
	uint16_t flagsOffset;
	uint8_t timeToLive;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t sourceAddr;
	uint32_t destAddr;
}ipv4_header_t;

void IPV4_DefineIP(uint32_t ip);
uint32_t IPV4_GetOwnIP(void);
int IPV4_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply);

#endif /* IPV4_H_ */
