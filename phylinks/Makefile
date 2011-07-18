-include ../common/common.mk

OBJS_TCP = phy_tcp.o multififo.o
OBJS_TTY = phy_tty.o multififo.o
CCFLAGS += -c -O0 -g3 -Wall
LDFLAGS += -Wl

phy_tty: $(OBJS_TTY)
	$(CC) $(LDFLAGS) -o ../bin/phy_tty $(OBJS_TTY)

phy_tcp: $(OBJS_TCP)
	$(CC) $(LDFLAGS) -o ../bin/phy_tcp $(OBJS_TCP)

phy_tcp.o: phy_tcp.c
	$(CC) $(CCFLAGS) phy_tcp.c -o phy_tcp.o

phy_tty.o: phy_tty.c
	$(CC) $(CCFLAGS) phy_tty.c -o phy_tty.o

multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

rebuild: clean all

all: phy_tcp phy_tty

clean:
	rm -f ../bin/phy_tcp $(OBJS_TCP) ../bin/phy_tty $(OBJS_TTY)
