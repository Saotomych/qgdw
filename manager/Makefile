-include ../common/common.mk

OBJS_MGR = autoconfig.o multififo.o
CCFLAGS += -c -O0 -g3 -Wall
LDFLAGS += -Wl

manager: $(OBJS_MGR)
	$(CC) $(LDFLAGS) -o ../bin/manager $(OBJS_MGR)

autoconfig.o: autoconfig.c
	$(CC) $(CCFLAGS) autoconfig.c -o autoconfig.o

multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

rebuild: clean all

all: manager

clean:
	rm -f ../bin/manager $(OBJS_MGR)

