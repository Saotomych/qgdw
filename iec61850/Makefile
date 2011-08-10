-include ../common/common.mk

OBJS += startiec.o virtualize.o ssd.o xml.o multififo.o
CCFLAGS += -c -O0 -g3 -Wall
LDFLAGS += -Wl

startiec: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/startiec $(OBJS)

startiec.o: startiec.c
	$(CC) $(CCFLAGS) startiec.c -o startiec.o
virtualize.o: virtualize.c
	$(CC) $(CCFLAGS) virtualize.c -o virtualize.o
ssd.o: ssd.c
	$(CC) $(CCFLAGS) ssd.c -o ssd.o
xml.o: startiec.c
	$(CC) $(CCFLAGS) xml.c -o xml.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

all: startiec

rebuild: clean all

clean:
	rm -f ../bin/startiec $(OBJS)
