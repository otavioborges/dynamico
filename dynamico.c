#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>

#include <signal.h>
#include <unistd.h>

#include "udp.h"
#include "eth.h"
#include "ipv4.h"

#define DEFAULT_DHCP_PORT   67
#define DEFAULT_BUFFER_SIZE 0xFFFF

bool working = true;
int dhcpSock = -1;

void signalHandler(int signum){
	switch(signum){
	case SIGINT:
		if(working){
			printf("Vaza!\n");
			working = false;
		}else{
			// forced exit
			if(dhcpSock > 0)
				close(dhcpSock);
			exit(0);
		}
		break;
	default:
		printf("Pegou sinal: %d, sei lá que isso!\n", signum);
		break;
	}
}

uint32_t network = (10 << 24) | (30 << 16);
uint8_t usedRange[32];
uint8_t firstRange = 10;
uint8_t lastRange = 30;

int main(int argc, char **argv){
	struct sockaddr_in broadcastAddr;
	struct sockaddr senderAddr;
	socklen_t sendsize = sizeof(senderAddr);
	uint16_t listenningPort = DEFAULT_DHCP_PORT;
	char recvBuffer[DEFAULT_BUFFER_SIZE];
	char sendBuffer[DEFAULT_BUFFER_SIZE];
	int recvMsgLength;

	memset(usedRange, 0xFF, 32);

	if(signal(SIGINT, signalHandler) == SIG_ERR){
		printf("Erro ao configurar SIGINT, saindo...\n");
		return -1;
	}

	dhcpSock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if(dhcpSock < 0){
		printf("Erro ao criar socket, erro nº: %d, saindo...\n", dhcpSock);
		return -2;
	}

	const char opt[] = "enp2s0";
	uint32_t optLen = strlen(opt);
	if(setsockopt(dhcpSock, SOL_SOCKET, SO_BINDTODEVICE, opt, optLen) < 0){
		printf("Erro ao selecionar IF '%s' ao socket, saindo...\n", opt);
		return -3;
	}

	struct ifreq ifDetails;
	strcpy(ifDetails.ifr_ifrn.ifrn_name, opt);
	if(ioctl(dhcpSock, SIOCGIFHWADDR, &ifDetails) < 0){
		printf("Erro ao ler MAC da if. '%s', saindo...\n", opt);
		return -4;
	}

	ETH_DefineMAC((uint8_t *)ifDetails.ifr_addr.sa_data);
	if(ioctl(dhcpSock, SIOCGIFADDR, &ifDetails) < 0){
		printf("Erro ao ler IP da if. '%s', saindo...\n", opt);
		return -4;
	}

	IPV4_DefineIP((ifDetails.ifr_addr.sa_data + 2));

	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = INADDR_ANY;
	broadcastAddr.sin_port = htons(DEFAULT_DHCP_PORT);//htons(listenningPort);

//	if(bind(dhcpSock, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0){
//		printf("Erro de bind, saindo...\n");
//		return -3;
//	}

	while(working){
		printf("Aguardando mensagem...\n");
		recvMsgLength = recvfrom(dhcpSock, recvBuffer, DEFAULT_BUFFER_SIZE, 0, &senderAddr, &sendsize);
		if(recvMsgLength < 0){
			printf("Erro de leitura nº: %d.\n", recvMsgLength);
		}else{
			UDP_ProcessPacket(recvBuffer, recvMsgLength, (uint8_t *)sendBuffer);
//			memcpy((uint8_t *)&recvMsg, recvBuffer, recvMsgLength);
//			recvBuffer[recvMsgLength] = '\0';
//			printf("Mensagem: \n %s \n\n", recvBuffer);
		}
	}

	close(dhcpSock);
	return 0;
}
