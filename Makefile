all : parsers

clean : parsers-clean


parsers : Makefile force
	cd parsers; $(MAKE) -f Makefile all
u-tsts : Makefile force
	cd u-tsts; $(MAKE) -f Makefile all
	
	

parsers-clean : Makefile force
	cd parsers; $(MAKE) -f Makefile clean
u-tsts-clean : Makefile force
	cd u-tsts; $(MAKE) -f Makefile clean



force : ;