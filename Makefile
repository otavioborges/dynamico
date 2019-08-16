.PHONY: clean

ARCH?=x86
CC?=gcc
CPP?=g++
AR?=ar

ifeq ($(ARCH),arm)
	C_FLAGS = -mcpu=cortex-m7 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -c -fPIC
else
	C_FLAGS = -c -fPIC
endif

BUILD_DIR = build/$(ARCH)
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
TARGET    = $(BUILD_DIR)/libdynamico.a

all: $(OBJ_FILES)
	ifeq ($(ARCH),arm)
		$(CROSS_COMPILE)$(AR) rcs $(TARGET) $^
	else
		$(CROSS_COMPILE)$(CC) $(TARGET) -o $^
	endif
$(BUILD_DIR)/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -I./inc -o $@ $<

#	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o eth.o eth.c
#	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o udp.o udp.c
#	gcc -c -fPIC -g3 -I./inc -o dynamico.o dynamico.c
#	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o dhcp.o dhcp.c
#	$(CROSS_COMPILE)$(CC) $(C_FLAGS) -c -fPIC -g3 -I./inc -o ipv4.o ipv4.c

#	$(CROSS_COMPILE)$(AR) rcs libdynamico.a eth.o udp.o dhcp.o ipv4.o
#	gcc -g3 -I. -o dynamico eth.o udp.o dynamico.o dhcp.o ipv4.o

clean:
	rm -rf *.o libdynamico.a
