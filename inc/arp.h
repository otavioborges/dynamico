/*
 * arp.h
 *
 *  Created on: Aug 26, 2019
 *      Author: root
 */

#ifndef INC_ARP_H_
#define INC_ARP_H_

#include <stdint.h>
#include "eth.h"

#define PACKET_ARP_LENGTH				28

typedef enum{
	ARP_OPER_REQUEST	= 0x0001,
	ARP_OPER_REPLY		= 0x0002
}arp_oper_t;

typedef struct{
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t oper;
	uint16_t sha[3];
	uint16_t spa[2];
	uint16_t tha[3];
	uint16_t tpa[2];
}arp_header_t;

int ARP_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *reply);
int ARP_CreateRequest(uint32_t ip, uint8_t *mac, uint8_t *payload);

#endif /* INC_ARP_H_ */
