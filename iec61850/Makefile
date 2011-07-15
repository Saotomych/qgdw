OBJS += startiec.o virtualize.o ssd.o xml.o multififo.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

startiec: $(OBJS)
	cc $(LDFLAGS) -o ../bin/startiec $(OBJS)

startiec.o: startiec.c
	cc $(CCFLAGS) startiec.c -o startiec.o
virtualize.o: virtualize.c
	cc $(CCFLAGS) virtualize.c -o virtualize.o
ssd.o: ssd.c
	cc $(CCFLAGS) ssd.c -o ssd.o
xml.o: startiec.c
	cc $(CCFLAGS) xml.c -o xml.o
multififo.o: ../common/multififo.c
	cc $(CCFLAGS) ../common/multififo.c -o multififo.o

all: startiec

rebuild: clean all

clean:
	-rm ../bin/startiec $(OBJS)
