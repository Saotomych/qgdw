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

// Speed chains for find connect
uint32_t sciec101[]={9600,2400,1200,0};
uint32_t scdlt645[]={9600,2400,1200,0};
uint32_t scm700[]={9600,0};

char *Addrfile;

// Create lowlevel string in tlstr buffer
int createllforlr(LOWREC *lr, uint16_t speed){
	tlstr[0] = 0;
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
LOWREC *lr;
FILE *f;

	f = fopen(fname, "w+");
	for (i=0; i<maxrec; i++){
		lr = lrs[i];
		if ((lr->copied & copy) | (~copy&1)) createllforlr(lr, lr->setspeed);
		len = strlen(tlstr);
		if (len) lr->scfg = malloc(len);
		strcpy(lr->scfg, tlstr);
		fputs(tlstr, f);
	}
	fputs("\n",f);
	fclose(f);

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
		lrs[maxrec]->scfg = 0;
		lrs[maxrec]->connect = 0;
		lrs[maxrec]->copied = 0;
		lrs[maxrec]->myep = 0;
		lrs[maxrec]->scen = 0;
		lrs[maxrec]->setspeed = 0;
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

int main(int argc, char * argv[]){
int i, allfiles, j;
// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");

	if (loadaddrcfg("addr.cfg") == -1) return -1;

	// Create lowrecord structures
	XMLSelectSource(Addrfile);

	// Create lowlevel.cfg for concrete level
	// 1: fixed asdu iec104
	// 2: fixed asdu iec101
	// 3: fixed MAC, dynamic asdu, m700
	// 4: dynamic asdu, dlt645
	for (i=0; i < MAXLEVEL; i++){
		// Create full tables for all variants of records in the file 'addr.cfg'.
		switch(lrs[i]->scen){
		case IEC104:
			createllforlr(lrs[i], 0);
			break;

		case IEC101:
			j=0;
			while(sciec101[i]){
				createllforlr(lrs[i], sciec101[i]);
			}
			break;

		case DLT645:
			j=0;
			while(sciec101[i]){
				createllforlr(lrs[i], sciec101[i]);
			}
			break;

		case MX00:
			createllforlr(lrs[i], 9600);
			break;
		}
	}

	createlrfile("/rw/mx00/configs/ll/lowlevel.1", FALSE);

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
