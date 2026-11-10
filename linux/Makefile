CC      = gcc
WARN    = -Wall -Wextra -Wno-format-security
COMMON  = -m32 -O0 -g -fno-stack-protector $(WARN)

all: chainboard chainboard-pie

chainboard: server.c
	$(CC) $(COMMON) -no-pie -z execstack -DPORT=8080 -o $@ $<

chainboard-pie: server.c
	$(CC) $(COMMON) -fPIE -pie -DPORT=8081 -o $@ $<

clean:
	rm -f chainboard chainboard-pie blockdata.bin blockdata.bak

.PHONY: all clean
