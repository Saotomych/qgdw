all : mkdirs unitlinks u-tsts phylinks iec61850 manager
	
clean : unitlinks-clean u-tsts-clean phylinks-clean iec61850-clean manager-clean

mkdirs :
	if test -d bin; then echo "directory bin already exists"; else mkdir bin; fi

unitlinks : Makefile force
	cd unitlinks; $(MAKE) -f Makefile all
u-tsts : Makefile force
	cd u-tsts; $(MAKE) -f Makefile all
phylinks: Makefile force
	cd phylinks; $(MAKE) -f Makefile all
iec61850: Makefile force
	cd iec61850; $(MAKE) -f Makefile all
manager: Makefile force
	cd manager; $(MAKE) -f Makefile all

unitlinks-clean : Makefile force
	cd unitlinks; $(MAKE) -f Makefile clean
u-tsts-clean : Makefile force
	cd u-tsts; $(MAKE) -f Makefile clean
phylinks-clean : Makefile force
	cd phylinks; $(MAKE) -f Makefile clean
iec61850-clean: Makefile force
	cd iec61850; $(MAKE) -f Makefile clean
manager-clean: Makefile force
	cd manager; $(MAKE) -f Makefile clean

force : ;
