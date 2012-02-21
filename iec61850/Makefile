-include ../common/common.mk

OBJS += startiec.o virtualize.o ssd.o xml.o multififo.o ts_print.o log_db.o
BDB_INC = -I../../BerkeleyDB.4.4/include
BDB_LIB = -L../../BerkeleyDB.4.4/lib
ifeq ($(ARCH), arm)
BDB_INC = 
BDB_LIB = 
endif
CCFLAGS += -c -O0 -g3 -Wall $(BDB_INC)
LDFLAGS += -Wl $(BDB_LIB)

startiec: $(OBJS)
	$(CC) $(LDFLAGS) -o ../bin/startiec $(OBJS) -lpthread -ldb 

startiec.o: startiec.c
	$(CC) $(CCFLAGS) startiec.c -o startiec.o
virtualize.o: virtualize.c
	$(CC) $(CCFLAGS) virtualize.c -o virtualize.o
ssd.o: ../common/ssd.c
	$(CC) $(CCFLAGS) ../common/ssd.c -o ssd.o
xml.o: ../common/xml.c
	$(CC) $(CCFLAGS) ../common/xml.c -o xml.o
multififo.o: ../common/multififo.c
	$(CC) $(CCFLAGS) ../common/multififo.c -o multififo.o
ts_print.o: ../common/ts_print.c
	$(CC) $(CCFLAGS) ../common/ts_print.c -o ts_print.o
log_db.o: log_db.c
	$(CC) $(CCFLAGS) log_db.c -o log_db.o

all: startiec

rebuild: clean all

clean:
	rm -f ../bin/startiec $(OBJS)
