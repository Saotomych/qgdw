all : parsers


parsers : Makefile force
	cd parsers; make -f Makefile all
u-tsts : Makefile force
	cd u-tsts; make -f Makefile all
	
	
clean : parsers-clean


parsers-clean : Makefile force
	cd parsers; make -f Makefile clean
u-tsts-clean : Makefile force
	cd u-tsts; make -f Makefile clean



force : ;