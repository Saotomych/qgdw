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
#include "../common/ts_print.h"
#include "../common/iec61850.h"
#include "../common/xml.h"
#include "log_db.h"

char prepath[] = {"/rw/mx00"};
char pathul[] = {"unitlinks"};
char pathphy[] = {"phyints"};
char pathmain[] = {"mainapp"};
char pathconfig[] = {"configs"};
char mainlink[] = {"main"};

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

// Program terminating
void sighandler(int arg){
	// close log env and db
	log_db_env_close();
	exit(0);
}

int main(int argc, char * argv[]){
pid_t chldpid;
char buf[5];
int lenname;
char *cidname;

	// Make config name
	lenname = strlen(prepath) + strlen(pathconfig) + strlen(IECCONFIG) + 3;
	cidname = malloc(lenname);
	ts_sprintf(cidname, "%s/%s/%s", prepath, pathconfig, IECCONFIG);

	// Parsing cid, create virtualization structures from common iec61850 configuration
	if (cid_build(cidname)){
		ts_printf(STDOUT_FILENO, "IEC61850: cid file not found\n");
		exit(1);
	}

	free(cidname);

	// Cross connection of IEC structures
	crossconnection();

	// Start of virtualize functions
	chldpid = virt_start("startiec");
	if (chldpid == -1){
		ts_printf(STDOUT_FILENO, "IEC61850: Virtualization wasn't started\n");
		exit(2);
	}

	ts_printf(STDOUT_FILENO, "\n--- Low level applications ready --- \n\n");

	signal(SIGALRM, catch_alarm);
	alarm(ALARM_PER);

	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

	// open log env and db
	log_db_env_open();

	// Cycle data routing in rcv_data
	do{
		mf_waitevent(buf, sizeof(buf), 0);
	}while(!appexit);

	mf_exit();

	// close log env and db
	log_db_env_close();

	return 0;
}

