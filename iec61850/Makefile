OBJS += startiec.o virtualize.o ssd.o xml.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

startiec: $(OBJS)
	cc $(LDFLAGS) -o startiec $(OBJS)

startiec.o: startiec.c
	cc $(CCFLAGS) startiec.c -o startiec.o
virtualize.o: virtualize.c
	cc $(CCFLAGS) virtualize.c -o virtualize.o
ssd.o: ssd.c
	cc $(CCFLAGS) ssd.c -o ssd.o
xml.o: startiec.c
	cc $(CCFLAGS) xml.c -o xml.o

rebuild: clean all

clean:
	-rm startiec $(OBJS)
