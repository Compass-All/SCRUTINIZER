CURRENT_DIR := $(CURDIR)
COMPILE_PATH := $(abspath $(CURRENT_DIR)/../..)

ifeq ($(shell uname -m),aarch64)
    CPP = gcc
else
    CPP = $(COMPILE_PATH)/dev_workspace/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
endif

CPPFLAGS = -g -static


SRC = sc_user_client.c sc_enclib.c sha256-armv8-aarch64.S aes128-armv8-aarch64.S
OBJ = $(SRC:.c=.o)
EXEC = sc_user_client

all: $(EXEC)

$(EXEC): $(SRC)
	$(CPP) $(CPPFLAGS) -o $(EXEC) $(SRC)

clean:
	rm -rf *.o $(EXEC)