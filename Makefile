all : mkdirs parsers u-tsts
	
clean : parsers-clean u-tsts-clean


mkdirs :
	if test -d bin; then echo "directory bin already exists"; else mkdir bin; fi

parsers : Makefile force
	cd parsers; $(MAKE) -f Makefile all
u-tsts : Makefile force
	cd u-tsts; $(MAKE) -f Makefile all
	
	

parsers-clean : Makefile force
	cd parsers; $(MAKE) -f Makefile clean
u-tsts-clean : Makefile force
	cd u-tsts; $(MAKE) -f Makefile clean



force : ;