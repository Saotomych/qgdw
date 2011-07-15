/*
 * startiec.c
 *
 *  Created on: 02.06.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "iec61850.h"
#include "xml.h"

char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char mainlink[] = {"main"};


struct config_device_list{
	LIST l;
	struct config_device cd;
};

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo
static sigset_t sigmask;

// Create structures according to ieclevel.ssd
int ssd_build(void){
char *SCLfile;
FILE *fssd;
int ssdlen, ret = 0;
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
	else ret = -1;

	return ret;
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
	do{
		sigsuspend(&sigmask);
	}while(!appexit);

	mf_exit();

	return 0;
}

