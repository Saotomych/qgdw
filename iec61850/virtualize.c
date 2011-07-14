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

// View dataset as ASDU for virtual IED
typedef struct _SCADA_ASDU{
	LIST l;
	LNODE *myln;
	uint32_t ASDUaddr;			// use find_by_int, get from IED.options
	u08  ASDUframe[SCADA_ASDU_MAXSIZE];
	ASDU_DATAMAP	*fdmap;
} SCADA_ASDU;


// Variables for asdu actions
static LIST fasdu;
static SCADA_ASDU *firstasdu;
static SCADA_ASDU *actasdu;
static int maxasdu;

int rcvdata(int len){

	return 0;
}

int rcvinit(ep_init_header *ih){

	return 0;
}

int asdu_parsparse(){
SCADA_ASDU *sa_new;
ASDU_DATAMAP *actdmap;
LNODE *aln = (LNODE*) &fln;
DOBJ *adobj;
DTYPE *adtype;
ATTR *dattr;

	firstasdu = (SCADA_ASDU*) &fasdu.next;
	actasdu = (SCADA_ASDU*) &fasdu;

	while(aln->l.next){
		aln = aln->l.next;
		if (aln->ln.options){
			sa_new = malloc(sizeof(SCADA_ASDU));
			if (sa_new){
				// Fill SCADA_ASDU
				actasdu->l.next = sa_new;
				sa_new->l.prev = actasdu;
				sa_new->l.next = 0;
				actasdu = sa_new;
				actasdu->myln = aln;
				actasdu->ASDUaddr = atoi(aln->ln.options);
				adobj = (DOBJ*) &fdo.next;
				actdmap = actasdu->fdmap;

				while(adobj->l.next){
//					if (adobj->dobj.name)
				}

				maxasdu++;
			}
		}
	}

	return 0;
}

int virt_start(){
pid_t chldpid;

	// TODO Building mapping meter asdu to ssd asdu

	// Run multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tcp", rcvdata, rcvinit);

	return chldpid;
}
