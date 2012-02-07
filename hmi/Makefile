-include ../common/common.mk

OBJS += hmi.o menu.o action_fn.o multififo.o ssd.o xml.o varcontrol.o 


NANOINCLUDE = ../../nano-x.X11.x86/microwin/src/include
NANOLIBS = ../../nano-x.X11.x86/microwin/src/libs

ifeq ($(ARCH), arm)
NANOINCLUDE = ../../nano-x.fb.uclibc.arm/microwin/src/include
NANOLIBS = ../../nano-x.fb.uclibc.arm/microwin/src/libs
endif

CCNANOFLAGS += -c -O0 -g3 -Wall -I $(NANOINCLUDE) -L $(NANOLIBS)
CCFLAGS += -c -O0 -g3 -Wall
LDFLAGS += -Wl -lnano-X

hmi: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/hmi700 $(OBJS)
hmi.o: hmi.c
	$(CC) $(CCNANOFLAGS) hmi.c -o hmi.o
menu.o: menu.c
	$(CC) $(CCNANOFLAGS) menu.c -o menu.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o
ssd.o: ../common/ssd.c
	$(CC) $(CCFLAGS) ../common/ssd.c -o ssd.o
xml.o: ../common/xml.c
	$(CC) $(CCFLAGS) ../common/xml.c -o xml.o
varcontrol.o: ../common/varcontrol.c
	$(CC) $(CCFLAGS) ../common/varcontrol.c -o varcontrol.o
action_fn.o: action_fn.c
	$(CC) $(CCFLAGS) action_fn.c -o action_fn.o

all: hmi

rebuild: clean all

clean:
	rm -f ../bin/hmi700 $(OBJS)
