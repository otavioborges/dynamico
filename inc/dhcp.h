/*
 * dhcp.h
 *
 *  Created on: Jun 17, 2019
 *      Author: root
 */

#ifndef DHCP_H_
#define DHCP_H_

#define DHCP_SERVER_PORT		0x4300
#define DHCP_MAX_NAME_LENGTH	64
#define DHCP_OVERHEAD 			236

#define REVERT_32_BITS(x)			(((x << 24) & 0xFF000000) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | ((x >> 24) & 0x000000FF))

#include <stdint.h>

typedef enum{
	ADDR_TYPE_ETHERNET 		= 1,
	ADDR_TYPE_IEEE_802		= 6,
	ADDR_TYPE_ARCNET		= 7,
	ADDR_TYPE_FRAME_RELAY	= 15,
	ADDR_TYPE_ATM			= 16,
	ADDR_TYPE_HDLC			= 17,
	ADDR_TYPE_FIBRE_CHANNEL	= 18,
	ADDR_TYPE_ATM2			= 19,
	ADDR_TYPE_SERIAL_LINE	= 20
}addr_type_t;

typedef struct{
	uint8_t op;
	uint8_t hType;
	uint8_t hLen;
	uint8_t hops;
	uint32_t XID;
	uint16_t secs;
	uint16_t flags;
	uint32_t cIAddr;
	uint32_t yIAddr;
	uint32_t sIAddr;
	uint32_t gIAddr;
	uint8_t cHAddr[16];
	char sName[64];
	char file[128];
	uint8_t options[2048];
}dhcp_header_t;

typedef struct{
	uint32_t ipAddr;
	uint32_t issueTime;
	uint8_t hwAddr[6];
	char name[DHCP_MAX_NAME_LENGTH];
}dhcp_lease_t;

typedef struct{
	uint8_t id;
	uint8_t length;
	uint8_t value;
}dhcp_option_t;

extern uint32_t g_serverAddr;

void DHCP_Init(uint32_t network, uint32_t netmask, uint32_t initialRange, uint32_t endRange, uint32_t server);
uint16_t DHCP_Parse(uint8_t *clientMAC, dhcp_header_t *msg, uint8_t *reply, uint32_t *broadcast);

#endif /* DHCP_H_ */
