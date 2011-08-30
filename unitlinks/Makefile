RELEASE = no

all : mkdirs iec104 iec101 dlt645 m700

clean : iec104-clean  iec101-clean dlt645-clean m700-clean


mkdirs :
	if test -d obj; then echo "directory obj already exists"; else mkdir obj; fi

iec104 : Makefile.iec104 force
	$(MAKE) -f Makefile.iec104 all RELEASE=$(RELEASE)

iec101 : Makefile.iec101 force
	$(MAKE) -f Makefile.iec101 all RELEASE=$(RELEASE)

dlt645 : Makefile.dlt645 force
	$(MAKE) -f Makefile.dlt645 all RELEASE=$(RELEASE)

m700 : Makefile.m700 force
	$(MAKE) -f Makefile.m700 all RELEASE=$(RELEASE)


iec104-clean : Makefile.iec104 force
	$(MAKE) -f Makefile.iec104 clean

iec101-clean : Makefile.iec101 force
	$(MAKE) -f Makefile.iec101 clean

dlt645-clean : Makefile.dlt645 force
	$(MAKE) -f Makefile.dlt645 clean

m700-clean : Makefile.m700 force
	$(MAKE) -f Makefile.m700 clean


force : ;
