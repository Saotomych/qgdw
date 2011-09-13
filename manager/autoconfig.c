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

int createlltable(void){
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

	// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");
	// Create full table for all addr.cfg records
	createlltable();

// Start endpoints for kipp

// Control of answers and forming new lowlevel.cfg

// Exit by time

// write new lowlevel.cfg

// Quit all lowlevel applications


// Start endpoints for m500/m700

// Control of answers and forming new lowlevel.cfg

// Exit by time

// write new lowlevel.cfg

// Quit all lowlevel applications


// Start endpoints for m100/m300

// Control of answers and forming new lowlevel.cfg

// Exit by time

// write new lowlevel.cfg

// Quit all lowlevel applications


}
