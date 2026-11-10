CC      = i686-w64-mingw32-gcc
CFLAGS  = -O0 -g -Wall -Wextra -fstack-protector-strong
LDFLAGS = -Wl,--image-base,0x11220000 \
          -Wl,--disable-dynamicbase \
          -Wl,--disable-nxcompat \
          -static
LDLIBS  = -lws2_32

all: chainboard.exe

chainboard.exe: server.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

gadgets: chainboard.exe
	@i686-w64-mingw32-nm chainboard.exe | grep -E "jmp_esp_gadget|ppr_gadget"

clean:
	rm -f chainboard.exe blockdata.bin

.PHONY: all gadgets clean
