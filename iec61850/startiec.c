/*
 * startiec.c
 *
 *  Created on: 02.06.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "iec61850.h"
#include "xml.h"

char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char mainlink[] = {"main"};

char *SCLfile;

// Create structures according to ieclevel.ssd
int ssd_build(void){
FILE *fssd;
int ssdlen;
struct stat fst;
 	// Get size of main config file
	if (stat("/rw/mx00/configs/ieclevel.ssd", &fst) == -1){
		printf("IEC: SCL file not found\n");
	}
	SCLfile = malloc(fst.st_size);
	// Loading main config file
	fssd = fopen("/rw/mx00/configs/ieclevel.ssd", "r");
	ssdlen = fread(SCLfile, 1, (size_t) (fst.st_size), fssd);
	if (ssdlen == fst.st_size) XMLSelectSource(SCLfile);
	else return -1;

	return 0;
}

int main(int argc, char * argv[]){
pid_t chldpid;

	// Parsing ssd, create virtualization structures from common iec61850 configuration
	if (ssd_build()){
		printf("IEC61850: SSD not found\n");
		exit(1);
	}

	// Start of virtualize functions
	chldpid = virt_start();
	if (chldpid == -1){
		printf("IEC: Virtualization don't started\n");
		exit(2);
	}

	// Cycle data routing in rcv_data

	mf_exit();

	return 0;
}

