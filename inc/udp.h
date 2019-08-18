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
	uint8_t options[40];
}udp_header_t;

int UDP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast);

#endif /* UDP_H_ */
