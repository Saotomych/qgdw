all : iec104 iec101 dlt645

clean : iec104-clean  iec101-clean


iec104 : Makefile.iec104 force
	$(MAKE) -f Makefile.iec104 all

iec101 : Makefile.iec101 force
	$(MAKE) -f Makefile.iec101 all

dlt645 : Makefile.dlt645 force
	$(MAKE) -f Makefile.dlt645 all


iec104-clean : Makefile.iec104 force
	$(MAKE) -f Makefile.iec104 clean

iec101-clean : Makefile.iec101 force
	$(MAKE) -f Makefile.iec101 clean

dlt645-clean : Makefile.dlt645 force
	$(MAKE) -f Makefile.dlt645 clean


force : ;
