/*
 * autoconfig.c
 *
 *  Created on: 01.09.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/multififo.h"

// Scenario types
#define KIPP104		1
#define KIPP101		2
#define MXDLT		3
#define MXINT		4

typedef struct lowrecord{
	char 		cfg[128];
	uint16_t	asdu;
	u08			connect;
	u08 		copied;
	uint32_t	ldinst;
	uint32_t	addr;
	uint32_t	addrdlt;
	uint16_t	port;
	uint16_t	scen;		// Case for type lowlevel string
} LOWREC;

LOWREC *lrs[MAXEP];

uint32_t maxrec = 0, actrec = 0;

uint16_t	lastasdu;
uint16_t	lastldinst;

// Create lowlevel.cfg for concrete level
// 1: fixed asdu
// 2: fixed MAC, dynamic asdu
// 3: dynamic asdu
// and concrete speed according to table
int createlltable(u08 level, u08 spdidx){
FILE *addrcfg;
struct phy_route *pr;
char *p, *pport;
char outbuf[256];
int i = 1;

	lastasdu = 1;
	lastldinst = 0;

// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(LOWREC) * MAXEP);
	addrcfg = fopen("/rw/mx00/configs/addr.cfg", "r");
	if (addrcfg){
		// Create table lowlevel.cfg
		// 1 - all for iec-101 & iec-104
		// 2 - m700 & m500 - 1 only
		// 3 - other
		// For all internal tty-devices: test for speed 9600, 2400, 1200
		// Create strings for lowlevel.cfg from 1 string of addr.cfg

	}else return -1;
	return 0;
}

int main(int argc, char * argv[]){
int i;
// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");

	for (i=0; i < MAXLEVEL; i++){
		// Create full table for all addr.cfg records
		createlltable();
		// Start endpoints

		// Control of answers and forming new lowlevel.cfg

		// Exit by time

		// write new lowlevel.cfg

		// Quit all lowlevel applications

	}

	rename("/rw/mx00/configs/lowlevel.bak", "/rw/mx00/configs/lowlevel.cfg");

}
