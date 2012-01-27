-include ../common/common.mk

OBJS += hmi.o menu.o multififo.o
NANOINCLUDE = /home/alex/PRJS/nano-x.X11.x86/microwin/src/include
NANOLIBS = /home/alex/PRJS/nano-x.X11.x86/microwin/src/libs
CCFLAGS += -c -O0 -g3 -Wall -I $(NANOINCLUDE) -L $(NANOLIBS)
LDFLAGS += -Wl -lnano-X

hmi: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/hmi700 $(OBJS)
hmi.o: hmi.c
	$(CC) $(CCFLAGS) hmi.c -o hmi.o
menu.o: menu.c
	$(CC) $(CCFLAGS) menu.c -o menu.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

all: hmi

rebuild: clean all

clean:
	rm -f ../bin/hmi700 $(OBJS)