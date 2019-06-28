.PHONY: clean

CROSS_COMPILE = /opt/gcc-arm-none-eabi-8-2018-q4-major/bin/arm-none-eabi-
CC = gcc
AR = ar

C_FLAGS = -mcpu=cortex-m7 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections

all:
	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o eth.o eth.c
	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o udp.o udp.c
#	gcc -c -fPIC -g3 -I./inc -o dynamico.o dynamico.c
	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o dhcp.o dhcp.c
	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o ipv4.o ipv4.c

	$(CROSS_COMPILE)$(AR) rcs libdynamico.a eth.o udp.o dhcp.o ipv4.o
#	gcc -g3 -I. -o dynamico eth.o udp.o dynamico.o dhcp.o ipv4.o

clean:
	rm -rf *.o libdynamico.a