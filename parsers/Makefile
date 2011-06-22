MAKEFILE = Makefile



all : iec104 iec101

iec104 : $(MAKEFILE).iec104 force
	$(MAKE) -f $(MAKEFILE).iec104 all

iec101 : $(MAKEFILE).iec101 force
	$(MAKE) -f $(MAKEFILE).iec101 all


clean : iec104-clean  iec101-clean

iec104-clean : $(MAKEFILE).iec104 force
	$(MAKE) -f $(MAKEFILE).iec104 clean

iec101-clean : $(MAKEFILE).iec101 force
	$(MAKE) -f $(MAKEFILE).iec101 clean

force : ;
