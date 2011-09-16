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

uint32_t maxrec = 0, actrec = 0;

uint16_t	lastasdu;
uint16_t	lastldinst;

// Speed chains for find connect
uint32_t spdstty4[] = {9600};
uint32_t spdstty3[]={9600,2400,1200,0};

uint32_t *spdscens[] = {NULL, NULL, spdstty3, spdstty4, spdstty3};

char *Addrfile;

uint16_t actasdu = 100;

// Support functions
void setdynamicasdu(LOWREC *lr){
u08 i = 0;
	while(lrs[i]){
		if (lrs[i]->asdu == actasdu){
			actasdu++;
			i = 0;
		}
		i++;
	}
	lr->asdu = actasdu;
	actasdu++;
}

// Create lowlevel strings in text buffer
int createllforiec104(LOWREC *lr, uint16_t speed, char *llstr){
// Example: 967 -addr 84.242.3.213 -port 2404 -name "unitlink-iec104" -mode CONNECT

	// Form lowlevel string
	// For energy meter used CONNECT mode only
	sprintf(llstr, "%d -addr %s -port %d -name \"unitlink-iec104\" -mode CONNECT\n", lr->asdu, inet_ntoa(lr->sai.sin_addr), lr->sai.sin_port);

	return 0;
}

int createllforiec101(LOWREC *lr, uint16_t speed, char *llstr){
// Example: 967 -name "unitlink-iec101" -port ttyS3 -pars 9600,8N1,NO

	// Form lowlevel string
	sprintf(llstr, "%d -name \"unitlink-iec101\" -port ttyS3 -pars %d,8E1,NO\n", lr->asdu, lr->setspeed);

	return 0;
}

int createllfordlt645(LOWREC *lr, uint16_t speed, char *llstr){
// Example: 1001 -addr 001083100021 -name "unitlink-dlt645" -port ttyS3 -pars 9600,8E1,NO
	// Form real ASDU
	setdynamicasdu(lr);

	// Form lowlevel string
	sprintf(llstr, "%d -addr %10ld -name \"unitlink-dlt645\" -port ttyS3 -pars %d,8E1,NO\n", lr->asdu, lr->addrdlt, lr->setspeed);

	return 0;
}

int createllformx00(LOWREC *lr, uint16_t speed, char *llstr){
// Example: 1000 -addr 01 -name "unitlink-m700" -port ttyS4 -pars 9600,8E1,NO
	// Form real ASDU
	setdynamicasdu(lr);

	// Form lowlevel string
	sprintf(llstr, "%d -addr 01 -name \"unitlink-m700\" -port ttyS4 -pars 9600,8E1,NO\n", lr->asdu);

	return 0;
}

int (*scenfunc[])(LOWREC *lr, uint16_t speed, char *llstr) = {NULL, createllforiec104, createllforiec101, createllformx00, createllfordlt645};

int createfirstfile(char *fname, u08 scen, uint16_t spds){
int i, len;
LOWREC *lr;
FILE *f;
char *tlstr = malloc(128);

	f = fopen(fname, "w+");
	for (i=0; i<maxrec; i++){
		lr = lrs[i];
		if (lr->scen == scen){
			if (scenfunc[scen](lr, spds, tlstr)){
				printf("Config Manager error: scenario error or don't create lowlevel.cfg\n");
				return -1;
			}
			len = strlen(tlstr) + 1;
			if (len) lr->scfg = malloc(len);
			strcpy(lr->scfg, tlstr);
			fputs(tlstr, f);
		}
	}
	fputs("\n",f);
	fclose(f);
	free(tlstr);

	return 0;
}

// Cycle for creating all records for one type and speed
int createlrfile(char *fname, u08 copy){
int ret = 0, i, len;
LOWREC *lr;
FILE *f;
char *tlstr = malloc(128);

	f = fopen(fname, "w+");
	for (i=0; i<maxrec; i++){
		lr = lrs[i];
		if ((lr->copied & copy) | (~copy&1))
			if (!scenfunc[lr->scen](lr, lr->setspeed, tlstr)) printf("Config Manager error: don't create lowlevel.cfg");
		len = strlen(tlstr);
		if (len) lr->scfg = malloc(len);
		strcpy(lr->scfg, tlstr);
		fputs(tlstr, f);
	}
	fputs("\n",f);
	fclose(f);
	free(tlstr);

	return ret;
}

// Create lowrecord, call from addrxml-parser
int createlowrecord(LOWREC *lr){
int ret = -1;

	lrs[maxrec] = malloc(sizeof(LOWREC));
	if (lrs[maxrec]){
		ret = 0;
		lrs[maxrec]->sai.sin_family = AF_INET;
		lrs[maxrec]->sai.sin_addr.s_addr = lr->sai.sin_addr.s_addr;
		lrs[maxrec]->sai.sin_port = lr->sai.sin_port;
		lrs[maxrec]->addrdlt = lr->addrdlt;
		lrs[maxrec]->asdu = lr->asdu;
		lrs[maxrec]->ldinst = lr->ldinst;
		lrs[maxrec]->scen = lr->scen;
		lrs[maxrec]->scfg = 0;
		lrs[maxrec]->connect = 0;
		lrs[maxrec]->copied = 0;
		lrs[maxrec]->myep = 0;
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

	printf("Config Manager: low records = %d\n", maxrec);
	// Create lowlevel.cfg for concrete speed level
	// 1: fixed asdu iec104
	// 2: fixed asdu iec101
	// 3: fixed MAC, dynamic asdu, m700
	// 4: dynamic asdu, dlt645
	for (scen = 1; scen < 5; scen++){
		spds = spdscens[scen];
		i = 0;
		do{
			// Create lowlevel cfg file for devices of concrete level and speed
			if (spds) createfirstfile("/rw/mx00/configs/lowlevel.cfg", scen, spds[i]);
			else createfirstfile("/rw/mx00/configs/lowlevel.cfg", scen, 0);

			printf("Config Manager: create lowlevel.cfg for scen %d\n", scen);

			// Start endpoints

			// Control of answers and forming new lowlevel.cfg

			// Exit by time

			// write new lowlevel.cfg

			// Quit all lowlevel applications

			// Create lowlevel cfg file for online devices
//			createlrfile("/rw/mx00/configs/lowlevel.cfg", FALSE);
			i++;
		}while((spds) && (spds[i]));
	}

//	rename("/rw/mx00/configs/lowlevel.bak", "/rw/mx00/configs/lowlevel.cfg");

	return 0;
}
