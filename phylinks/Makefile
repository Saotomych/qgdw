OBJS_TCP = phy_tcp.o multififo.o
OBJS_TTY = phy_tty.o multififo.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

phy_tty: $(OBJS_TTY)
	cc $(LDFLAGS) -o ../bin/phy_tty $(OBJS_TTY)

phy_tcp: $(OBJS_TCP)
	cc $(LDFLAGS) -o ../bin/phy_tcp $(OBJS_TCP)

phy_tcp.o: phy_tcp.c
	cc $(CCFLAGS) phy_tcp.c -o phy_tcp.o

phy_tty.o: phy_tty.c
	cc $(CCFLAGS) phy_tty.c -o phy_tty.o

multififo.o: ../common/multififo.c
	cc $(CCFLAGS) ../common/multififo.c -o multififo.o

rebuild: clean all

all: phy_tcp phy_tty

clean:
	rm -f phy_tcp $(OBJS_TCP) phy_tty $(OBJS_TTY)
