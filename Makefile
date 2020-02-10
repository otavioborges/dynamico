.PHONY: clean

ARCH?=x86
CC?=gcc
CPP?=g++
AR?=ar

ifeq ($(ARCH),arm)
	C_FLAGS = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -c -fPIC -g3
else
	C_FLAGS = -c -fPIC -g3 -DLOGGING=0
endif

ifeq ($(ARCH),arm)
	COMP = gcc
else
	COMP = g++
endif

BUILD_DIR = build/$(ARCH)
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
TARGET    = $(BUILD_DIR)/libdynamico.a

all: directories $(TARGET)

directories:
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ_FILES)
	$(CROSS_COMPILE)$(AR) -crs $(TARGET) $^

$(BUILD_DIR)/%.o: %.c
	$(CROSS_COMPILE)$(COMP) $(C_FLAGS) -I./inc -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)
