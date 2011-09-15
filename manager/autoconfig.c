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
uint32_t spdstty4[] = {9600};
uint32_t spdstty3[]={9600,2400,1200,0};

uint32_t *scens = {0, 0, spdstty3, spdstty3, spdstty4};

char *Addrfile;

// Create lowlevel strings in text buffer
int createllforiec104(LOWREC *lr, uint16_t speed){

	return 0;
}

int createllforiec101(LOWREC *lr, uint16_t speed){

	return 0;
}

int createllfordlt645(LOWREC *lr, uint16_t speed){

	return 0;
}

int createllformx00(LOWREC *lr, uint16_t speed){

	return 0;
}

int (*scenfunc[])(LOWREC *lr, uint16_t speed) = {createllforiec104, createllforiec101, createllfordlt645, createllformx00};

int createfirstfile(char *fname, u08 scen, uint16_t spds){
int ret = 0, i, len;
LOWREC *lr;
FILE *f;

	f = fopen(fname, "w+");
	for (i=0; i<maxrec; i++){
		lr = lrs[i];
		if (lr->scen == scen)
			if (!scenfunc[scen](lr, spds)) printf("Task Manager error: don't create lowlevel.cfg");
		len = strlen(tlstr);
		if (len) lr->scfg = malloc(len);
		strcpy(lr->scfg, tlstr);
		fputs(tlstr, f);
	}
	fputs("\n",f);
	fclose(f);

	return ret;
}

// Cycle for creating all records for one type and speed
int createlrfile(char *fname, u08 copy){
int ret = 0, i, len;
LOWREC *lr;
FILE *f;

	f = fopen(fname, "w+");
	for (i=0; i<maxrec; i++){
		lr = lrs[i];
		if ((lr->copied & copy) | (~copy&1))
			if (!scenfunc[scen](lr, spds)) printf("Task Manager error: don't create lowlevel.cfg");
		len = strlen(tlstr);
		if (len) lr->scfg = malloc(len);
		strcpy(lr->scfg, tlstr);
		fputs(tlstr, f);
	}
	fputs("\n",f);
	fclose(f);

	return ret;
}

// Create lowrecord, call from addrxml-parser
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
u08 i, scen;
uint32_t *spds;
// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");

	if (loadaddrcfg("addr.cfg") == -1) return -1;

	// Create lowrecord structures
	XMLSelectSource(Addrfile);

	// Create lowlevel.cfg for concrete speed level
	// 1: fixed asdu iec104
	// 2: fixed asdu iec101
	// 3: dynamic asdu, dlt645
	// 4: fixed MAC, dynamic asdu, m700
	for (scen = 1; scen < 5; scen++){
		spds = scens[scen];
		i = 0;
		while(spds[i]){
			// Create lowlevel cfg file for devices of concrete level and speed
			createfirstfile("/rw/mx00/configs/ll/lowlevel.cfg", scen, spds[i]);

			// Create lowlevel cfg file for online devices
			createlrfile("/rw/mx00/configs/ll/lowlevel.cfg", FALSE);
			i++;
		}
	}

	// Substitution of level.??? to lowlevel.cfg

	// Start endpoints

	// Control of answers and forming new lowlevel.cfg

	// Exit by time

	// write new lowlevel.cfg

	// Quit all lowlevel applications

	rename("/rw/mx00/configs/lowlevel.bak", "/rw/mx00/configs/lowlevel.cfg");

	return 0;
}
