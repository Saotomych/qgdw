all : mkdirs unitlinks u-tsts
	
clean : unitlinks-clean u-tsts-clean


mkdirs :
	if test -d bin; then echo "directory bin already exists"; else mkdir bin; fi

unitlinks : Makefile force
	cd unitlinks; $(MAKE) -f Makefile all
u-tsts : Makefile force
	cd u-tsts; $(MAKE) -f Makefile all
	
	

unitlinks-clean : Makefile force
	cd unitlinks; $(MAKE) -f Makefile clean
u-tsts-clean : Makefile force
	cd u-tsts; $(MAKE) -f Makefile clean



force : ;