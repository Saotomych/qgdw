-include ../common/common.mk

OBJS += hmi.o menu.o action_fn.o menu_fn.o winparser.o multififo.o ssd.o xml.o varcontrol.o tarif.o ts_print.o

NANOINCLUDE = ../../nano-x.X11.x86/microwin/src/include
NANOLIBS = ../../nano-x.X11.x86/microwin/src/lib

ifeq ($(ARCH), arm)
NANOINCLUDE = ../../nano-x.fb.uclibc.arm/microwin/src/include
NANOLIBS = ../../nano-x.fb.uclibc.arm/microwin/src/lib
endif

CCFLAGS += -c -O0 -g3 -Wall -I $(NANOINCLUDE)
LDFLAGS += -Wl -L $(NANOLIBS) -lnano-X

hmi: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/hmi700 $(OBJS) -lnano-X -lpthread
hmi.o: hmi.c
	$(CC) $(CCFLAGS) hmi.c -o hmi.o
menu.o: menu.c
	$(CC) $(CCFLAGS) menu.c -o menu.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o
ts_print.o: ../common/ts_print.c
	$(CC) $(CCFLAGS) ../common/ts_print.c -o ts_print.o
ssd.o: ../common/ssd.c
	$(CC) $(CCFLAGS) ../common/ssd.c -o ssd.o
xml.o: ../common/xml.c
	$(CC) $(CCFLAGS) ../common/xml.c -o xml.o
varcontrol.o: ../common/varcontrol.c
	$(CC) $(CCFLAGS) ../common/varcontrol.c -o varcontrol.o
tarif.o: ../common/tarif.c
	$(CC) $(CCFLAGS) ../common/tarif.c -o tarif.o
action_fn.o: action_fn.c
	$(CC) $(CCFLAGS) action_fn.c -o action_fn.o
menu_fn.o: menu_fn.c
	$(CC) $(CCFLAGS) menu_fn.c -o menu_fn.o
winparser.o: winparser.c
	$(CC) $(CCFLAGS) winparser.c -o winparser.o
ts_print.o: ../common/ts_print.c
	$(CC) $(CCFLAGS) ../common/ts_print.c -o ts_print.o
	

all: hmi

rebuild: clean all

clean:
	rm -f ../bin/hmi700 $(OBJS)
