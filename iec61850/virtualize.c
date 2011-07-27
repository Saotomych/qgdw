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
#include "iec61850.h"

#define SCADA_ASDU_MAXSIZE 512

// DATA
typedef struct _DATAMAP{
	LIST l;
	DOBJ		*mydobj;
	uint32_t	scadaid;		// scadaid  = position in scada asdu, get from DOBJ.options
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu
	uint8_t		value_type; 	// for "on fly" converting, get from DOTYPE.ATTR.stVal <-> DOTYPE.ATTR.btype
} ASDU_DATAMAP;

typedef struct _SCADA_TYPE{		// Analog Logical Node Type
	LIST l;
	LNTYPE *mylntype;
	ASDU_DATAMAP	*fdmap;
} SCADA_ASDU_TYPE;

// View dataset as ASDU for virtual IED
typedef struct _SCADA_ASDU{		// Analog Logical Node
	LIST l;
	LNODE *myln;
	uint32_t ASDUaddr;			// use find_by_int, get from IED.options
	u08  ASDUframe[SCADA_ASDU_MAXSIZE];
	SCADA_ASDU_TYPE *myscadatype;
} SCADA_ASDU;

// Pointer to full mapping config as text
char *MCFGfile;

// Variables for asdu actions
static LIST fasdu, fasdutype;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *newlist;
	plist->next = malloc(size);
	if (!plist->next){
		printf("IEC: malloc error:%d - %s\n",errno, strerror(errno));
		exit(3);
	}

	newlist = plist->next;
	newlist->prev = plist;
	newlist->next = 0;
	return newlist;
}

// Get mapping parameters from special config file 'mainmap.cfg' of real ASDU frames from meters
int get_map_by_name(char *name, uint32_t *mid){
char *p;

	p = strstr(MCFGfile, name);
	if (!p) return -1;
	while((*p != 0xA) && (p != MCFGfile)) p--;
	while(*p <= '0') p++;
	*mid = atoi(p);

	return 0;
}

// Find DA with name and ptr equals to name and ptr DTYPE
// Return int equal string typedef
int get_type_by_name(char *name, char *type){
DTYPE *adtype;
ATTR *dattr = (ATTR*) &fattr;
char *p1;
int ret = 0;

	// Find DTYPE by type
	adtype = (DTYPE*) fdtype.next;
	while(adtype){
		if (!strcmp(adtype->dtype.id, type)) break;
		adtype = adtype->l.next;
	}

	if (adtype){
		// Find ATTR by name & ptr to type
		dattr = (ATTR*) fattr.next;
		while((dattr) && (!ret)){
			if (&adtype->dtype == dattr->attr.pmydatatype){
				// Own type
				if (!strcmp(dattr->attr.name, name)){
					// Name yes. Detect btype and convert to INT
					p1 = dattr->attr.btype;
					if (strstr(p1, "INT32")) ret = 1;
					else ret = -1;
				}
			}
			dattr = dattr->l.next;
		}
	}else ret = -1;

	return ret;
}

int rcvdata(int len){
char *buff;
int adr, dir;

	buff = malloc(len);

	if(!buff) return -1;

	mf_readbuffer(buff, len, &adr, &dir);
//#ifdef _DEBUG
	printf("ASDU: Data received. Address = %d, Length = %d, Direction = %s.\n", adr, len, dir == DIRDN? "DIRUP" : "DIRDN");
//#endif
	free(buff);

	return 0;
}

int asdu_parser(void){
SCADA_ASDU *actasdu;
SCADA_ASDU_TYPE *actasdutype;
ASDU_DATAMAP *actasdudm;

LIST fdm;

LNODE *aln;
LNTYPE *alnt;
DOBJ *adobj;

	printf("ASDU: Start ASDU mapping to parse\n");

	// Create SCADA_ASDU_TYPE list
	actasdutype = (SCADA_ASDU_TYPE*) &fasdutype;
	alnt = (LNTYPE*) flntype.next;
	while(alnt){

		actasdutype = create_next_struct_in_list((LIST*) actasdutype, sizeof(SCADA_ASDU_TYPE));

		printf("ASDU: new SCADA_ASDU_TYPE\n");

				// Fill SCADA_ASDU_TYPE
		actasdutype->mylntype = alnt;

		// create ASDU_DATAMAP list
		actasdudm = (ASDU_DATAMAP*) &fdm;
		adobj = (DOBJ*) fdo.next;

		while(adobj){
			if ((adobj->dobj.options) &&
				 (adobj->dobj.pmynodetype == &alnt->lntype)){
					// creating new DATAMAP and filling
					actasdudm = create_next_struct_in_list((LIST*) actasdudm, sizeof(ASDU_DATAMAP));
					// Fill ASDU_DATAMAP
					actasdudm->mydobj = adobj;
					actasdudm->scadaid = atoi(adobj->dobj.options);
					if (!get_map_by_name(adobj->dobj.name, &actasdudm->meterid)){
						// find by DOType->DA.name = stVal => DOType->DA.btype
						actasdudm->value_type = get_type_by_name("stVal", adobj->dobj.type);
						printf("ASDU: new SCADA_DO for DOBJ name=%s type=%s: %d =>moveto=> %d by type=%d\n",
								adobj->dobj.name, adobj->dobj.type, actasdudm->scadaid, actasdudm->meterid, actasdudm->value_type);
					}else printf("ASDU: new SCADA_DO for DOBJ error: Tag not found into mainmap.cfg");
			}else printf("ASDU: new SCADA_DO for DOBJ (without mapping) name=%s type=%s\n", adobj->dobj.name, adobj->dobj.type);

			// Next DOBJ
			adobj = adobj->l.next;
		}

		printf("ASDU: ready SCADA_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		alnt = alnt->l.next;
	}


	// Create SCADA_ASDU list
	actasdu = (SCADA_ASDU*) &fasdu;
	aln = (LNODE*) fln.next;
	while(aln){
		if (aln->ln.options){
			actasdu = create_next_struct_in_list((LIST*) actasdu, sizeof(SCADA_ASDU));

			// Fill SCADA_ASDU
			actasdu->myln = aln;
			actasdu->ASDUaddr = atoi(aln->ln.options);

			// Link to SCADA_ASDU_TYPE
			// Find LNTYPE.id = SCADA_ASDU_TYPE.LN.lntype
			actasdutype =  (SCADA_ASDU_TYPE*) fasdutype.next;
			while(actasdutype){
				alnt = actasdutype->mylntype;
				if (!strcmp(alnt->lntype.id, aln->ln.lntype)) break;
				actasdutype = actasdutype->l.next;
			}
			if (actasdutype){
				actasdu->myscadatype = actasdutype;
				printf("ASDU: SCADA_ASDU %s.%s.%s linked to TYPE %s\n",
						actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass, actasdutype->mylntype->lntype.id);
			}
			else printf("ASDU: SCADA_ASDU %s.%s.%s NOT linked to TYPE\n", actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass);
		}

		printf("ASDU: new SCADA_ASDU addr=%d for LN name=%s.%s.%s type=%s ied=%s\n",
				actasdu->ASDUaddr, aln->ln.ldinst, aln->ln.lninst, aln->ln.lnclass, aln->ln.lntype, aln->ln.iedname);

		aln = aln->l.next;
	}

	printf("ASDU: End ASDU mapping\n");

	return 0;
}

// TODO Remove Test config after debugging
char mainapp[] 		= {"startiec"};
char unitlink[] 	= {"unitlink-iec104"};
char physlink[] 	= {"phy_tcp"};

struct config_device cd = {
		mainapp,
		unitlink,
		physlink,
		55555
};

int virt_start(char *appname){
FILE *fmcfg;
int clen, ret;
struct stat fst;

char *p;

SCADA_ASDU *sasdu = (SCADA_ASDU *) &fasdu;

struct _LNODETYPE 	*plntype;
DOBJ	*pdobj;

pid_t chldpid;

// Read mainmap.cfg into memory
	if (stat("/rw/mx00/configs/mainmap.cfg", &fst) == -1){
		printf("IEC Virt: 'mainmap.cfg' file not found\n");
	}
	MCFGfile =  malloc(fst.st_size);
	fmcfg = fopen("/rw/mx00/configs/mainmap.cfg", "r");
	clen = fread(MCFGfile, 1, (size_t) (fst.st_size), fmcfg);
	if (clen != fst.st_size) ret = -1;
	else{
		// Building mapping meter asdu to ssd asdu
		if (asdu_parser()) ret = -1;
		else{
			// Run multififo
			chldpid = mf_init("/rw/mx00/mainapp", appname, rcvdata);
			ret = chldpid;
		}
	}
	free(MCFGfile);
	printf("\n--- Configuration ready --- \n\n");

//	Execute all low level application for devices by LNodes
	sasdu = sasdu->l.next;
	while(sasdu){

		printf("\n--------------\nIEC Virt: execute for LNode %s, id asdu = %s\n", sasdu->myln->ln.lninst, sasdu->myln->ln.options);

		// Create config_device
		cd.protoname = malloc(strlen(sasdu->myln->ln.lninst) + 1);
		strcpy(cd.protoname, sasdu->myln->ln.lninst);
		p = cd.protoname;
		while((*p != '.') && (*p)) p++;
		*p = 0;
		cd.phyname = p + 1;
		cd.name = appname;
		cd.addr = sasdu->ASDUaddr;

		// New endpoint
		mf_newendpoint(&cd, "/rw/mx00/unitlinks", 0);

		free(cd.protoname);
		sleep(1);	// Delay for

		sasdu = sasdu->l.next;
	};

// Setup of unitlinks for getting DATA OBJECTS
	// get SCADA_ASDU => get LN_TYPE => get DATA_OBJECT list => write list to unitlink
//	sasdu = ((SCADA_ASDU *) &fasdu)->l.next;
//	while(sasdu){
//		// find logical node type
//		plntype = sasdu->myln->ln.pmytype;
//
//		// find data_object list
//		pdobj = plntype->pfdobj;
//
//		// enumerate data objects
//
//		// get pointer to data_object list
//
//		// create buffer of datatypes
//
//		// write to endpoint
//
//		sasdu = sasdu->l.next;
//	}

	return ret;
}
