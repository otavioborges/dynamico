/*
 * icmp.h
 *
 *  Created on: 26 de dez de 2019
 *      Author: otavio
 */

#ifndef INC_ICMP_H_
#define INC_ICMP_H_

#include <stdint.h>
#include "ipv4.h"

#define PACKET_UDP_LENGTH	8

typedef struct{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t restOfHeader;
}icmp_header_t;

typedef enum{
	ICMP_TYPE_ECHO_REPLY		= 0,
	ICMP_TYPE_DEST_UNREACHABLE	= 3,
	ICMP_TYPE_SOURCE_QUENCH		= 4,
	ICMP_TYPE_REDIRECT_MSG		= 5,
	ICMP_TYPE_ECHO_REQUEST		= 8,
	ICMP_TYPE_ROUTER_ADV		= 9,
	ICMP_TYPE_ROUTER_SOL		= 10,
	ICMP_TYPE_TIME_EXCEEDED		= 11,
	ICMP_TYPE_PARAM_PROBLEM		= 12,
	ICMP_TYPE_TIMESTAMP			= 13,
	ICMP_TYPE_TIMESTAMP_REPLY	= 14,
	ICMP_TYPE_INFO_REQ			= 15,
	ICMP_TYPE_INFO_REPLY		= 16,
	ICMP_TYPE_ADDR_MASK_REQ		= 17,
	ICMP_TYPE_ADDR_MASK_REPLY	= 18,
	ICMP_TYPE_TRACEROUTE		= 30,
	ICMP_TYPE_EXT_ECHO_REQ		= 43
}icmp_type_t;

int ICMP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast);
int ICMP_GetHeader(uint8_t *payload, uint16_t dataLength, uint16_t destPort, uint16_t srcPort);
int ICMP_SendPacket(uint32_t destination, uint8_t *data, icmp_type_t type, uint8_t code, uint32_t length, uint8_t *payload);

#endif /* INC_ICMP_H_ */
