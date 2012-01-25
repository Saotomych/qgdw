-include ../common/common.mk

OBJS += hmi.o menu.o multififo.o
CCFLAGS += -c -O0 -g3 -Wall
LDFLAGS += -Wl

hmi700: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/hmi $(OBJS)

hmi.o: hmi.c
	$(CC) $(CCFLAGS) hmi.c -o hmi.o
menu.o: menu.c
	$(CC) $(CCFLAGS) menu.c -o menu.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

all: hmi

rebuild: clean all

clean:
	rm -f ../bin/hmi $(OBJS)
