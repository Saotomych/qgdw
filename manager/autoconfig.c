/*
 * autoconfig.c
 *
 *  Created on: 01.09.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "autoconfig.h"

LOWREC *lrs[MAXEP];

char tlstr[128];

uint32_t maxrec = 0, actrec = 0;

uint16_t	lastasdu;
uint16_t	lastldinst;

char *Addrfile;

// Create string for lrs[idx]
int createllforlr(LOWREC *lr){
	switch (lr->scen){
	case IEC104:

		break;

	case IEC101:

		break;

	case DLT645:

		break;

	case MX00:

		break;

	default:
		return -1;
	}
	return 0;
}

// Cycle for creating all records for one type and speed
int createlrfile(char *fname, u08 copy){
int ret, i, len;

	for (i=0; i<maxrec; i++){
		tlstr[0] = 0;
		if ((lrs[i]->copied & copy) | (~copy&1)) createllforlr(lrs[i]);
		len = strlen(tlstr);
		if (len) lrs[i]->scfg = malloc(len);
	}

	return ret;
}

int createlowrecord(LOWREC *lr){
int ret = -1;

	lrs[maxrec] = malloc(sizeof(LOWREC));
	if (lrs[maxrec]){
		ret = 0;
		lrs[maxrec]->addr = lr->addr;
		lrs[maxrec]->addrdlt = lr->addrdlt;
		lrs[maxrec]->asdu = lr->asdu;
		lrs[maxrec]->ldinst = lr->ldinst;
		lrs[maxrec]->port = lr->port;
		lrs[maxrec]->cfg[0] = 0;
		lrs[maxrec]->connect = 0;
		lrs[maxrec]->copied = 0;
		lrs[maxrec]->myep = 0;
		lrs[maxrec]->scen = -1;
		maxrec++;
	}

	return ret;
}

int loadaddrcfg(char *name){
FILE *addrcfg;
int adrlen, ret = -1;
struct stat fst;

	addrcfg = fopen("/rw/mx00/configs/addr.cfg", "r");
	if (addrcfg){
	 	// Get size of main config file
		if (stat("/rw/mx00/configs/addr.cfg", &fst) == -1){
			printf("IEC61850: Addr.cfg file not found\n");
		}

		Addrfile = malloc(fst.st_size);

		// Loading main config file
		addrcfg = fopen("/rw/mx00/configs/addr.cfg", "r");
		adrlen = fread(Addrfile, 1, (size_t) (fst.st_size), addrcfg);
		if (adrlen == fst.st_size) ret = 0;
	}

	return ret;
}

// Create lowlevel.cfg for concrete level
// 1: fixed asdu
// 2: fixed MAC, dynamic asdu
// 3: dynamic asdu
// and concrete speed according to table
int createlltables(u08 level, u08 spdidx){
	lastasdu = 1;
	lastldinst = 0;

	// Create tables in format lowlevel.cfg
	// by names: lowlevel.<level asdu>.<type>.<speed>
	// 1 - all for iec-101 & iec-104
	// 2 - m700 & m500 - 1 only
	// 3 - other
	// Create string for lowlevel.cfg from 1 string of addr.cfg
	XMLSelectSource(Addrfile);

	return 0;
}

int main(int argc, char * argv[]){
int i, allfiles;
// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");

	if (loadadrcfg("addr.cfg") == -1) return -1;

	for (i=0; i < MAXLEVEL; i++){
		// Create full tables for all variants of records in the file 'addr.cfg'.
		createlltables();
	}

	allfiles = 0;
	for (i=0; i < allfiles; i++){
		// Substitution of level.??? to lowlevel.cfg

		// Start endpoints

		// Control of answers and forming new lowlevel.cfg

		// Exit by time

		// write new lowlevel.cfg

		// Quit all lowlevel applications

	}

	rename("/rw/mx00/configs/lowlevel.bak", "/rw/mx00/configs/lowlevel.cfg");

	return 0;
}
