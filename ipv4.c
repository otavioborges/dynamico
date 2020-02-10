/*
 * ipv4.c
 *
 *  Created on: Jun 18, 2019
 *      Author: root
 */

#define IPV4_HOSTS_LENGTH		32

#include "ipv4.h"
#include "udp.h"
#include <stdio.h>
#include <string.h>

static uint32_t IPV4_OWN_IP		= 0;
static uint32_t IPV4_NETMASK	= 0;
static uint32_t IPV4_GATEWAY	= 0;

static ipv4_host_t IPV4_HOSTS[IPV4_HOSTS_LENGTH] = {
	{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},
	{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},
	{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},
	{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0},{{0,0,0,0,0,0}, 0}
};

void IPV4_DefineIP(uint32_t ip, uint32_t netmask, uint32_t gateway){
	IPV4_OWN_IP = ip;
	IPV4_NETMASK = netmask;
	IPV4_GATEWAY = gateway;
}

uint32_t IPV4_GetOwnIP(void){
	return IPV4_OWN_IP;
}

uint32_t IPV4_GetNetmask(void){
	return IPV4_NETMASK;
}

uint32_t IPV4_GetGateway(void){
	return IPV4_GATEWAY;
}

static uint32_t IPV4_CalculateSubnetBroadcast(void){
	return IPV4_OWN_IP | ~IPV4_NETMASK;
}

int IPV4_ProcessPacket(uint8_t *msg, uint16_t length, uint8_t *sourceMAC, uint8_t *reply){
	ipv4_header_t *ipv4;
	ipv4_header_t *replyIpv4 = (ipv4_header_t *)reply;
	uint8_t headerLength = 0;

	ipv4 = (ipv4_header_t *)msg;
	if((ipv4->versionIHL >> 4) != 4)
		return ETH_ERROR_WRONG_IP_VERSION;	// TODO: support ipv6

	headerLength = (ipv4->versionIHL & 0x0F) * 4;

	replyIpv4->versionIHL = ipv4->versionIHL;
	replyIpv4->type = 0x10;//ipv4->type;
	replyIpv4->identification = 0;//ipv4->identification;
	replyIpv4->flagsOffset = 0;
	replyIpv4->timeToLive = ipv4->timeToLive;
	replyIpv4->protocol = IPv4_PROTOCOL_UDP;
	replyIpv4->checksum = 0;
	replyIpv4->sourceAddr = REVERT_32_BITS(IPV4_OWN_IP);

#if (DYN_CONF_DHCP_CLIENT == 1)
	if(IPV4_OWN_IP == 0)
		ipv4->destAddr = 0xFFFFFFFF;	// allow to receive packets when IP is not defined
#endif

#if(DYN_CONF_ACCEPT_IPv4_BROADCAST == 1)
	uint8_t accept = 0;
	if(ipv4->destAddr == IPV4_IP_BROADCAST)
		accept = 1;
	else if((REVERT_32_BITS(ipv4->destAddr) & IPV4_NETMASK) == (IPV4_OWN_IP & IPV4_NETMASK))
		accept = 1;
	else if(REVERT_32_BITS(ipv4->destAddr) == IPV4_OWN_IP)
		accept = 1;
	else
		accept = 0;
#else
	if(REVERT_32_BITS(ipv4->destAddr) == IPV4_OWN_IP)
		accept = 1;
	else
		accept = 0;
#endif
	if(accept){
		uint8_t *payload = (reply + headerLength);
		int responseLength = 0;
		uint32_t replyOnBroadcast = 0;
		msg += headerLength;
		length -= headerLength;

		if(ipv4->protocol == IPv4_PROTOCOL_UDP){
			responseLength = UDP_ProcessPacket(msg, length, payload, sourceMAC, &replyOnBroadcast);
		}else if(ipv4->protocol == IPv4_PROTOCOL_TCP){
			responseLength = 0;	// TODO: implement TCP
		}else if(ipv4->protocol == IPv4_PROTOCOL_ICMP){
			responseLength = 0;
		}else{
			return ETH_ERROR_PROT_NOT_SUPPORTED;
		}

		if(responseLength == 0){
			return 0;
		}else{
			if(replyOnBroadcast == IPV4_REPLY_ON_BROADCAST_ALL)
				replyIpv4->destAddr = IPV4_IP_BROADCAST;
			else if(replyOnBroadcast == IPV4_REPLY_ON_BROADCAST_SUBNET)
				replyIpv4->destAddr = REVERT_32_BITS(IPV4_CalculateSubnetBroadcast());
			else if(replyOnBroadcast == IPV4_REPLY_TO_SENDER)
				replyIpv4->destAddr = ipv4->sourceAddr;
			else	// reply to DHCP new IP
				replyIpv4->destAddr = REVERT_32_BITS(replyOnBroadcast);

			replyIpv4->length = REVERT_16BITS(responseLength + PACKET_IPv4_LENGTH);
			return (responseLength + headerLength);
		}
	}else{
		return ETH_ERROR_WRONG_DEST_IP;
	}
}

void IPV4_PrintIP(uint32_t address, char *buffer){
	uint8_t token = (uint8_t)(address & 0xFF);
	char octet[4];

	sprintf(buffer, "%d.%d.%d.%d", ((uint8_t)(address & 0xFF)),((uint8_t)((address >> 8) & 0xFF)),
			((uint8_t)((address >> 16) & 0xFF)),((uint8_t)((address >> 24) & 0xFF)));
}

uint32_t IPV4_atoi(char *ip){
	uint32_t response = 0;
	uint32_t token = 0;
	uint32_t idx = 0;

	while(ip[idx] != '\0' && idx < 256){
		if(ip[idx] == '.'){
			response += token;
			token = 0;
			response = response << 8;
		}else if((ip[idx] >= '0') && (ip[idx] <= '9')){
			token *= 10;
			token += (ip[idx] - '0');
		}else{
			return 0;
		}

		idx++;
	}

	if(token != 0)
		response += token;

	return response;
}

void IPV4_AddHost(uint8_t *mac, uint32_t ip){
	const static uint8_t noMac[6] = {0,0,0,0,0,0};

	for(uint16_t idx = 0; idx < IPV4_HOSTS_LENGTH; idx++){
		if(memcmp(IPV4_HOSTS[idx].mac, noMac, 6) == 0){
			// free space
			memcpy(IPV4_HOSTS[idx].mac, mac, 6);
			IPV4_HOSTS[idx].ip = ip;
			return;
		}
	}
}

uint8_t IPV4_HasHost(uint32_t ip){
	for(uint16_t idx = 0; idx < IPV4_HOSTS_LENGTH; idx++){
		if(ip == IPV4_HOSTS[idx].ip)
			return 1;
	}

	// not found
	return 0;
}

int IPV4_GetHeader(uint8_t *payload, uint32_t dataLength, uint32_t destination, ipv4_protocol_t protocol){
	ipv4_header_t *ipv4 = (ipv4_header_t *)payload;

	ipv4->versionIHL = 0x45;
	ipv4->type = 0x10;
	ipv4->identification = 0;
	ipv4->flagsOffset = 0;
	ipv4->timeToLive = 64;
	ipv4->protocol = protocol;
	ipv4->checksum = 0;
	ipv4->sourceAddr = REVERT_32_BITS(IPV4_OWN_IP);
	ipv4->destAddr = REVERT_32_BITS(destination);
	ipv4->length = REVERT_16BITS((dataLength + PACKET_IPv4_LENGTH));

	return PACKET_IPv4_LENGTH;
}

uint8_t *IPV4_GetMAC(uint32_t ip){
	// check if IP inside the network
	if((ip & IPV4_NETMASK) != (IPV4_OWN_IP & IPV4_NETMASK)){
		// outside, use GATEWAY MAC
		ip = IPV4_GATEWAY;
	}

	for(uint32_t idx = 0; idx < IPV4_HOSTS_LENGTH; idx++){
		if(IPV4_HOSTS[idx].ip == ip)
			return IPV4_HOSTS[idx].mac;
	}

	// not found!
	return NULL;
}
