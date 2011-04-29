obj-m := am160160.o
am160160-objs := drivers/am160160_drv.o drivers/uc1698.o drivers/st7529.o

clean: 
	rm -rf *.o *.cmd *.mod.c drivers/*.o
