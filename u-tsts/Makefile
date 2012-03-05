-include ../common/common.mk

OBJS1 = multififo-test.o multififo.o
OBJS2 = multififo-test-device.o multififo.o
OBJS3 = fbmmap.o
CCFLAGS += -c -Os -Wall
LDFLAGS += -Wl

multififo-test: $(OBJS1)
	$(CC) $(LDFLAGS) -o mainapptest $(OBJS1)

multififo-test-device: $(OBJS2)
	$(CC) $(LDFLAGS) -o devlinktest $(OBJS2)
	
fbmmap: $(OBJS3)
	$(CC) $(LDFLAGS) -o fbmmap $(OBJS3)

multififo-test.o: multififo-test.c
	$(CC) $(CCFLAGS) multififo-test.c -o multififo-test.o

multififo-test-device.o: multififo-test-device.c
	$(CC) $(CCFLAGS) multififo-test-device.c -o multififo-test-device.o

multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o
	
fbmmap.o: fbmmap.c
	$(CC) $(CCFLAGS) fbmmap.c -o fbmmap.o

rebuild: clean all

all: multififo-test multififo-test-device 

clean:
	rm -f mainapptest devlinktest $(OBJS1) $(OBJS2) fbmmap $(OBJS3)

force : ;
	