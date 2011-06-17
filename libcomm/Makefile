MAKEFILE = Makefile



all : libiec libdlt bin

libiec : $(MAKEFILE).libiec force
	$(MAKE) -f $(MAKEFILE).libiec all

libdlt : $(MAKEFILE).libdlt force
	$(MAKE) -f $(MAKEFILE).libdlt all

bin : $(MAKEFILE).bin force
	$(MAKE) -f $(MAKEFILE).bin all




clean : libiec-clean libdlt-clean bin-clean

libiec-clean : $(MAKEFILE).libiec force
	$(MAKE) -f $(MAKEFILE).libiec clean

libdlt-clean : $(MAKEFILE).libdlt force
	$(MAKE) -f $(MAKEFILE).libdlt clean

bin-clean : $(MAKEFILE).bin force
	$(MAKE) -f $(MAKEFILE).bin clean

force : ;
