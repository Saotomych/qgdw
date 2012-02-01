/*
 * startiec.c
 *
 *  Created on: 02.06.2011
 *      Author: Alex AVAlon
 */

#include <sys/time.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "../common/xml.h"

char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char mainlink[] = {"main"};

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

int main(int argc, char * argv[]){
pid_t chldpid;
char buf[5];

	// Parsing ssd, create virtualization structures from common iec61850 configuration
	if (ssd_build()){
		printf("IEC61850: SSD not found\n");
		exit(1);
	}
	// Cross connection of IEC structures
	crossconnection();

	// Start of virtualize functions
	chldpid = virt_start("startiec");
	if (chldpid == -1){
		printf("IEC61850: Virtualization wasn't started\n");
		exit(2);
	}

	printf("\n--- Low level applications ready --- \n\n");

	signal(SIGALRM, catch_alarm);
	alarm(ALARM_PER);

	// Cycle data routing in rcv_data
	do{
		mf_waitevent(buf, sizeof(buf), 0);
	}while(!appexit);

	mf_exit();

	return 0;
}

