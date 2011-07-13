/*
 * startiec.c
 *
 *  Created on: 02.06.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "iec61850.h"

char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char mainlink[] = {"main"};

char *SCLfile;

// Create structures according to ieclevel.ssd
int ssd_compile(void){
FILE *fssd;
char *p;
struct stat fst;
 	// Get size of main config file
	if (stat("/rw/mx00/configs/ieclevel.ssd", &fst) == -1){
		printf("IEC: SCL file not found\n");
	}
	SCLfile = malloc(fst.st_size);
	// Loading main config file
	fssd = fopen("/rw/mx00/configs/ieclevel.ssd", "r");
	p = fread(SCLfile, 1, fst.st_size, fssd);
	do{
		if (p){
//			if (strstr(cstr,"IED")) 		create_ied(cstr);			// call parse ied
//			if (strstr(cstr,"LNode")) 		create_ln(cstr);			// call parse ln
//			if (strstr(cstr,"DO")) 			create_data(cstr);			// call parse data
//			if (strstr(cstr,"DOType")) 		create_datatype(cstr);		// call parse data_type
//			if (strstr(cstr,"EnumType")) 	create_enum(cstr);			// call parse enum
//			if (strstr(cstr,"Substation"))	create_subst(cstr);			// call parse substation
		}
	}while(p);

	return 0;
}

// Link main config structures
void ssd_link(void){
//	link_datatypes();
//	link_datasets();
//	virt_ieds();
//	virt_lns();
//	virt_data();
//	virt_datatypes();
//	virtlink_datatypes();
}
FILE *ssd;

	// Loading main config file

int main(int argc, char * argv[]){
pid_t chldpid;

	// Parsing ssd, create virtualization structures from common iec61850 configuration
	if (ssd_compile()){
		printf("IEC61850: SSD not found\n");
		exit(1);
	}
	ssd_link();

	// Start of virtualize functions
//	chldpid = start_virtualize();
	if (chldpid == -1){
		printf("IEC61850: Virtualization don't started\n");
		exit(2);
	}

	// Cycle data routing in rcv_data


	return 0;
}

