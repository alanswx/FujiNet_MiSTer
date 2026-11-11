# s2cartd: the Studio II FujiNet cart's core0 for the MiSTer HPS.
#   make                     host build (macOS/Linux), for the Verilator sim
#   make CROSS=arm-none-linux-gnueabihf-   the MiSTer (static)

FW     ?= ../fujinet-firmware/pico
CROSS  ?=
CC      = $(CROSS)gcc
CFLAGS ?= -O2 -Wall -Wno-unused-function
CFLAGS += -std=gnu11 -I$(FW)/studio2/firmware/include -I$(FW)/common/include -I$(FW)/common/emu \
          -I$(FW)/studio2/firmware/baked
LDFLAGS ?=
ifneq ($(CROSS),)
LDFLAGS += -static
endif

SRC = s2cartd.c \
      $(FW)/common/src/fujimail.c $(FW)/common/src/fujibus.c $(FW)/common/src/fuji_crc.c \
      $(FW)/common/emu/fujitcp.c \
      $(FW)/studio2/firmware/src/s2map.c $(FW)/studio2/firmware/src/fuji_load.c \
      $(FW)/studio2/firmware/src/s2_text.c $(FW)/studio2/firmware/src/s2_font.c

OUT ?= s2cartd

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f s2cartd s2cartd-arm

.PHONY: clean
