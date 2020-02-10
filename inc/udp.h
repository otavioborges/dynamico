/*
 * udp_header.h
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#ifndef UDP_H_
#define UDP_H_

#include <stdint.h>
#include "ipv4.h"

#define PACKET_UDP_LENGTH	8

typedef struct{
	uint16_t sourcePort;
	uint16_t destPort;
	uint16_t length;
	uint16_t checksum;
}udp_header_t;

typedef int (*udp_callback_t)(uint8_t *, uint16_t , uint8_t *);

int UDP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast);
int UDP_GetHeader(uint8_t *payload, uint16_t dataLength, uint16_t destPort, uint16_t srcPort);
int UDP_SendPacket(uint32_t destination, uint8_t *data, uint16_t srcPort, uint16_t destPort, uint32_t length, uint8_t *payload, udp_callback_t callback);

#endif /* UDP_H_ */
