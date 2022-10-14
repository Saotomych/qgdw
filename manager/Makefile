-include ../common/common.mk

OBJS_MGR = autoconfig.o addrxml.o icdxml.o multififo.o ts_print.o getpaths.o
OBJS_PP = persparser.o
CCFLAGS += -c -O0 -g3 -Wall

persparser: $(OBJS_PP)
	$(CC) $(LDFLAGS) -o ../bin/persparser $(OBJS_PP)

persparser.o: persparser.c
	$(CC) $(CCFLAGS) persparser.c -o persparser.o
	
manager: $(OBJS_MGR)
	$(CC) $(LDFLAGS) -o ../bin/manager $(OBJS_MGR) -lpthread

autoconfig.o: autoconfig.c
	$(CC) $(CCFLAGS) autoconfig.c -o autoconfig.o

addrxml.o: addrxml.c
	$(CC) $(CCFLAGS) addrxml.c -o addrxml.o

icdxml.o: icdxml.c
	$(CC) $(CCFLAGS) icdxml.c -o icdxml.o

multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o

ts_print.o: ../common/ts_print.c
	$(CC) $(CCFLAGS) ../common/ts_print.c -o ts_print.o

getpaths.o: ../common/getpaths.c
	$(CC) $(CCFLAGS) ../common/getpaths.c -o getpaths.o

rebuild: clean all

all: manager persparser

clean:
	rm -f ../bin/manager $(OBJS_MGR)
	rm -f ../bin/persparser $(OBJS_PP)

