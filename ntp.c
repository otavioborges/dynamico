/*
 * ntp.c
 *
 *  Created on: 28 de dez de 2019
 *      Author: otavio
 */

#define NTP_TIMESTAMP_DELTA 2208988800U

#include "ntp.h"
#include "eth.h"
#include "ipv4.h"
#include "udp.h"

 __attribute__((weak)) void NTP_Callback(uint32_t timestamp){
	 // nothing to do here!
 }

int NTP_Parse(ntp_header_t *msg, uint8_t *reply){
	uint32_t epoch;
	if((msg->li_vn_mode & 0xC0) != 0){ // return with warnings
		return -1;
	}

	epoch = REVERT_32_BITS(msg->rxTm_s) - NTP_TIMESTAMP_DELTA;
	NTP_Callback(epoch);
	return 0;
}

int NTP_SendPacket(uint32_t destination, uint8_t *payload, uint32_t originTS, uint32_t originFraction){
	uint32_t totalLength = 0;
	uint8_t *macDest;

	macDest = IPV4_GetMAC(destination);
	totalLength = ETH_CreateHeader(payload, macDest, ETH_TYPE_IPV4);
	totalLength += IPV4_GetHeader((payload + totalLength), (PACKET_NTP_LENGTH + PACKET_UDP_LENGTH), destination, IPv4_PROTOCOL_UDP);
	totalLength += UDP_GetHeader((payload + totalLength), PACKET_NTP_LENGTH, NTP_UDP_PORT, NTP_UDP_PORT);

	ntp_header_t *ntp = (ntp_header_t *)(payload + totalLength);
	ntp->li_vn_mode = 0xe3;	// fixed for client request
	ntp->stratum = 2;
	ntp->poll = 6; // 64 seconds
	ntp->precision = 0xe9;

	ntp->rootDelay = 0;
	ntp->rootDispersion = 0;
	ntp->refId = 0;

	ntp->refTm_s = 0;
	ntp->refTm_f = 0;

	ntp->origTm_s = 0;
	ntp->origTm_f = 0;

	ntp->rxTm_s = 0;
	ntp->rxTm_f = 0;

	ntp->txTm_s = REVERT_32_BITS(originTS);
	ntp->txTm_f = REVERT_32_BITS(originFraction);

	return (totalLength + PACKET_NTP_LENGTH);
}
