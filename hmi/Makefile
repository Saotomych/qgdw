-include ../common/common.mk

OBJS += hmi.o menu.o multififo.o

NANOINCLUDE = ../../nano-x.X11.x86/microwin/src/include
NANOLIBS = ../../nano-x.X11.x86/microwin/src/lib

ifeq ($(ARCH), arm)
NANOINCLUDE = ../../nano-x.fb.uclibc.arm/microwin/src/include
NANOLIBS = ../../nano-x.fb.uclibc.arm/microwin/src/lib
endif

CCFLAGS += -c -O0 -g3 -Wall -I $(NANOINCLUDE)
LDFLAGS += -Wl -L $(NANOLIBS)

hmi: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/hmi700 $(OBJS) -lnano-X
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
