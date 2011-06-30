OBJS_TCP = phy_tcp.o multififo.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

phy_tcp: $(OBJS_TCP)
	cc $(LDFLAGS) -o phy_tcp $(OBJS_TCP)

phy_tcp.o: phy_tcp.c
	cc $(CCFLAGS) phy_tcp.c -o phy_tcp.o

multififo.o: ../common/multififo.c
	cc $(CCFLAGS) ../common/multififo.c -o multififo.o

rebuild: clean all

all: phy_tcp

clean:
	rm -f phy_tcp $(OBJS_TCP)
