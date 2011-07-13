/*
 * virtualize.c
 *
 *  Created on: 12.07.2011
 *      Author: alex AVAlon
 *
 *  This module make virtualization procedure for all data_types going from/to unitlinks
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"

struct config_device_list{
	struct config_device cd;
	struct config_device *prev;
	struct config_device *next;
};

#define SCADA_ASDU_MAXSIZE 512

// Mapping meter to scada asdu
struct data_map{
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu, meter.val = value according to meter.value_type
	char		valname[10];	// use find_by_string
	uint32_t	scadaid;		// scadaid  = position in scada asdu
};

// iec61850 - temporary, DATA
struct _DATA{
	char	name[10];

	union {
		uint32_t	ui;			/* unsigned integer representation */
		int32_t		i;			/* integer representation */
		float		f;			/* float representation */
	} value;					/* transferring value (e.g. measured value, counter readings, etc.) */

	uint8_t			value_type; /* e.g. integer, unsigned integer, float, boolean, etc. (need this for "on fly" protocol converting) */
};

// Set of data for one logical node (LN)
typedef struct data_list{
	struct _DATA	dat;
	void			*next;
	void			*prev;
} DATA;

// Set of data for any LN and IED in really
// There: for SCADA_ASDU
typedef struct _DATASET{
	char 			name[20];
	DATA 	*fdat;
	int 			maxdata;
} DATASET;

// View dataset as ASDU for virtual IED
struct _SCADA_ASDU{
	int ASDUaddr;			// use find_by_int
	u08  ASDUframe[SCADA_ASDU_MAXSIZE];
	char iedname[80];		// use find_by_string
	DATASET ds;
};

typedef struct asdu_list{
	struct _SCADA_ASDU sa;
	void *next;
} SCADA_ASDU;

// Variables for asdu actions
SCADA_ASDU *firstasdu;
SCADA_ASDU *actasdu;
DATA *lastdata;
int maxasdu;

// find struct in struct_list have 1 pars = val
void *find_by_int(int val){
char *p = 0;

	return p;
}

int rcvdata(int len){

	return 0;
}

int rcvinit(ep_init_header *ih){

	return 0;
}

//void ssd2asdu_parse(char *str, int mode){
//SCADA_ASDU *prev, *act=0;
//DATA *data=0;
//int i;
//char *p;
//
//	switch(mode){
//	case 1: // parse "asdu"
//		if (!maxasdu){
//			firstasdu = malloc(sizeof(SCADA_ASDU));
//			if (firstasdu){
//				act = firstasdu;
//				act->next = 0;
//				maxasdu++;
//			}
//		}else{
//			prev = actasdu;
//			prev->next = malloc(sizeof(SCADA_ASDU));
//			if (prev->next) {
//				act = (SCADA_ASDU*) prev->next;
//				act->next = 0;
//				maxasdu++;
//			}
//		}
//
//		if (act){
//			// init ASDUaddr
//			p = strstr(str, "asdu");
//			if (p){
//				while(*p != '"') p++;
//				p++;
//				// copy asdu addr
//				act->sa.ASDUaddr = atoi(p);
//			}
//
//			// init NAME
//			p = strstr(str, "name");
//			if (p){
//				while(*p != '"') p++;
//				p++; i = 0;
//				// copy iedname
//				while(*p > ' '){
//					act->sa.iedname[i] = *p;
//					i++; p++;
//				}
//			}
//
//			// Init DATASET
//			act->sa.ds.maxdata = 0;
//			act->sa.ds.fdat = 0;
//			strcpy(act->sa.ds.name, "scada_dataset");
//
//			// Set actual asdu
//			actasdu = act;
//			printf("IEC: Create new scada_asdu %d\n", maxasdu-1);
//		}else  printf("IEC: Error of creating new scada_asdu %d\n", maxasdu-1);
//
//		break;
//
//	case 2: // parse "asdupos"
//		if (actasdu){
//			act = actasdu;
//			if (!act->sa.ds.maxdata){
//				act->sa.ds.fdat = malloc(sizeof(DATA));
//				if (act->sa.ds.fdat){
//					data = act->sa.ds.fdat;
//					data->next = 0;
//					data->prev = 0;
//					act->sa.ds.maxdata++;
//				}
//			}else{
//				data = lastdata;
//				data->next = malloc(sizeof(DATA));
//				if (data->next){
//					data = (DATA*) data->next;
//					data->next = 0;
//					data->prev = 0;
//					act->sa.ds.maxdata++;
//				}
//			}
//
//			if (data){
//				// init NAME
//				p = strstr(str, "name");
//				if (p){
//					while(*p != '"') p++;
//					p++; i = 0;
//					// copy iedname
//					while(*p > ' '){
//						data->dat.name[i] = *p;
//						i++; p++;
//					}
//				}
//			}
//		}
//
//		// Set last setting data
//		lastdata = data;
//		printf("IEC: Create new data for scada\n");
//
//		break;
//
//	}
//
//
//}


int virt_start(){
pid_t chldpid;

	// TODO Building mapping meter asdu to ssd asdu

	// Run multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	return chldpid;
}
