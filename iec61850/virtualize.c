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

struct config_device_list{
	struct config_device cd;
	struct config_device *prev;
	struct config_device *next;
};

#define SCADA_ASDU_MAXSIZE 512

// DATA
typedef struct _DATAMAP{
	LIST l;
	DOBJ		*mydobj;
	uint32_t	scadaid;		// scadaid  = position in scada asdu, get from DOBJ.options
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu
	uint8_t		value_type; 	// for "on fly" converting, get from DOTYPE.ATTR.stVal <-> DOTYPE.ATTR.btype
} ASDU_DATAMAP;

typedef struct _SCADA_TYPE{
	LIST l;
	LNTYPE *mylntype;
	ASDU_DATAMAP	*fdmap;
} SCADA_ASDU_TYPE;

// View dataset as ASDU for virtual IED
typedef struct _SCADA_ASDU{
	LIST l;
	LNODE *myln;
	uint32_t ASDUaddr;			// use find_by_int, get from IED.options
	u08  ASDUframe[SCADA_ASDU_MAXSIZE];
	SCADA_ASDU_TYPE *myscadatype;
} SCADA_ASDU;


// Variables for asdu actions
static LIST fasdu, fasdutype;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *new;
	plist->next = malloc(size);

	if (!plist->next){
		printf("IEC: malloc error:%d - %s\n",errno, strerror(errno));
		exit(3);
	}

	new = plist->next;
	new->prev = plist;
	new->next = 0;
	return new;
}


int get_map_by_name(char *name, uint32_t *mid, uint32_t *sid){

	*mid = 4117;
	*sid = 517;

	return 0;
}

int get_type_by_name(char *name, struct _DTYPE *dt){

	return 150;
}

int rcvdata(int len){

	return 0;
}

int rcvinit(ep_init_header *ih){

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
//DTYPE *adtype;
//ATTR *dattr;
//char *p;

	printf("ASDU: Start ASDU mapping parse\n");

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
						if (!get_map_by_name(adobj->dobj.name, &actasdudm->meterid, &actasdudm->scadaid)){
							// find by DOType->DA.name = stVal => DOType->DA.btype
							actasdudm->value_type = get_type_by_name("stVal", adobj->dobj.pmydatatype);
						}
			}
			// Next DOBJ

			printf("ASDU: new SCADA_DO_TYPE for DTYPE name=%s type=%s\n", adobj->dobj.name, adobj->dobj.type);

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

			// TODO Link to SCADA_ASDU_TYPE

		}

		printf("ASDU: ready SCADA_ASDU addr=%d for LN name=%s.%s.%s type=%s ied=%s\n",
				actasdu->ASDUaddr, aln->ln.ldinst, aln->ln.lnclass, aln->ln.lninst, aln->ln.lntype, aln->ln.iedname);

		aln = aln->l.next;
	}

	printf("ASDU: Stop ASDU mapping parse\n");

	return 0;
}

int virt_start(){
pid_t chldpid;

	// TODO Building mapping meter asdu to ssd asdu
	if (asdu_parser()) exit(1);

	// Run multififo
//	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	return chldpid;
}
