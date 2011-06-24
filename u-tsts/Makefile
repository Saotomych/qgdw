OBJS1 = multififo-test.o multififo.o
OBJS2 = multififo-test-device.o multififo.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

multififo-test: $(OBJS1)
	cc $(LDFLAGS) -o mainapptest $(OBJS1)

multififo-test-device: $(OBJS2)
	cc $(LDFLAGS) -o devlinktest $(OBJS2)

multififo-test.o: multififo-test.c
	cc $(CCFLAGS) multififo-test.c -o multififo-test.o

multififo-test-device.o: multififo-test-device.c
	cc $(CCFLAGS) multififo-test-device.c -o multififo-test-device.o

multififo.o: ../common/multififo.c
	cc $(CCFLAGS) ../common/multififo.c -o multififo.o

rebuild: clean all

all: multififo-test multififo-test-device

clean:
	-rm mainapptest devlinktest $(OBJS1) $(OBJS2)
