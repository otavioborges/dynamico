/*
 * tcp.h
 *
 *  Created on: Jun 28, 2019
 *      Author: otavio
 */

#ifndef INC_TCP_H_
#define INC_TCP_H_

#include <stdint.h>

typedef struct{
	uint16_t sourcePort;
	uint16_t destPort;
	uint16_t length;
	uint16_t checksum;
	uint8_t options[40];
}tcp_header_t;

int TCP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply, uint8_t *sourceMaC, uint32_t *replyOnBroadcast);
int TCP_SendOurData(uint8_t *reply, uint8_t *msg, uint32_t length);

#endif /* INC_TCP_H_ */
