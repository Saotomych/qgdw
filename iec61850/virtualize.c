/*
 * virtualize.c
 *
 *  Created on: 12.07.2011
 *      Author: alex AVAlon
 *
 *  This module make virtualization procedure for all data_types going from/to unitlinks
 *
 */

#include <sys/time.h>
#include "../common/common.h"
#include "../common/multififo.h"
#include "../common/asdu.h"
#include "iec61850.h"

#define VIRT_ASDU_MAXSIZE 	512

// DATA
typedef struct _DATAMAP{
	LIST l;
	DOBJ		*mydobj;
	uint32_t	scadaid;		// scadaid  = position in scada asdu, get from DOBJ.options
	uint32_t	meterid;		// use find_by_int, meterid = position in meter asdu
	uint8_t		value_type; 	// for "on fly" converting, get from DOTYPE.ATTR.stVal <-> DOTYPE.ATTR.btype
} ASDU_DATAMAP;

typedef struct _SCADA_ASDU_TYPE{		// Analog Logical Node Type
	LIST l;
	LNTYPE *mylntype;
	ASDU_DATAMAP	*fdmap;
} VIRT_ASDU_TYPE;

// View dataset as ASDU for virtual IED
typedef struct _VIRT_ASDU{		// Analog Logical Node
	LIST l;
	LNODE *myln;
	VIRT_ASDU_TYPE *myasdutype;
} VIRT_ASDU;

typedef struct _SCADA_CH{
	LIST l;
	LDEVICE *myld;
	uint32_t ASDUaddr;			// use find_by_int, get from IED.options
	uint32_t baseoffset;		// Base offset of meter data in iec104 channel
} SCADA_CH;

typedef struct _SCADA_ASDU{
	LIST l;
	VIRT_ASDU *pscada;
} SCADA_ASDU;

// Pointer to full mapping config as text
char *MCFGfile;

// Variables for asdu actions
static LIST fasdu, fasdutype, fdm, fscada;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *newlist;
	plist->next = malloc(size);
	if (!plist->next){
		printf("IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
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
char *buff, *sendbuff;
int adr, dir, rdlen, fullrdlen;
int offset, res;
struct timeval tv;
ep_data_header *edh, *sedh;
data_unit *pdu, *spdu;
VIRT_ASDU *sasdu = (VIRT_ASDU*) fasdu.next;
asdu *pasdu, *psasdu;
ASDU_DATAMAP *pdm;
SCADA_ASDU *actscada;

	buff = malloc(len);

	if(!buff) return -1;

	fullrdlen = mf_readbuffer(buff, len, &adr, &dir);

//#ifdef _DEBUG
//	printf("IEC61850: Data received. Address = %d, Length = %d  %s.\n", adr, len, dir == DIRDN? "from down" : "from up");

	// set offset to zero before loop
	offset = 0;

	while(offset < fullrdlen){
		if(fullrdlen - offset < sizeof(ep_data_header)){
			printf("IEC61850: Found not full ep_data_header\n");
			free(buff);
			return 0;
		}

		edh = (struct ep_data_header *) (buff + offset);				// set start structure

		switch(edh->sys_msg){
		case EP_USER_DATA:
			rdlen = edh->len;
			pasdu = (asdu*) (buff + offset + sizeof(ep_data_header));
			rdlen -= sizeof(asdu);

			// find VIRT_ASDU
			while ((sasdu) && (sasdu->ASDUaddr != edh->adr)) sasdu = sasdu->l.next;
			if (!sasdu){
				printf("IEC61850 error: Address ASDU %d not found\n", edh->adr);
				free(buff);
				return 0;
			}

			printf("IEC61850: Values for ASDU = %d received\n", edh->adr);

			// TODO time synchronization, broadcast request, etc.

			// Making up Buffer for send to SCADA_ASDU.
			// It has data objects having mapping only
			sendbuff = malloc(len);
			sedh = (ep_data_header*) sendbuff;
			sedh->sys_msg = EP_USER_DATA;
			sedh->len = sizeof(asdu);

			psasdu = (asdu*) (sendbuff + sizeof(ep_data_header));
			memcpy(psasdu, pasdu, sizeof(asdu));
			psasdu->size = 0;
			spdu = (data_unit*) ((void*)psasdu + sizeof(asdu));
			pdu = (void*) pasdu + sizeof(asdu);

			while(rdlen > 0){
				if (pdu->id <= (VIRT_ASDU_MAXSIZE - 4)){
	 				// TODO Copy variable to data struct IEC61850
					// TODO Find type of variable and convert type on fly

					// Mapping id
					pdm = sasdu->myasdutype->fdmap;
					while ((pdm) && (pdm->meterid != pdu->id)) pdm = pdm->l.next;
					if (pdm){
						// TODO Find type of variable & Convert type on fly
						// Remap variable pdu->id -> id (for SCADA_ASDU)
						pdu->id =  sasdu->baseoffset + pdm->scadaid;
						memcpy(spdu, pdu, sizeof(data_unit));
						spdu++;
						psasdu->size++;
						sedh->len += sizeof(data_unit);
						printf("IEC61850: Value = 0x%X. id %d map to SCADA_ASDU id %d. Time = %d\n", pdu->value.ui, pdm->meterid, pdm->scadaid, pdu->time_tag);
					}
//					else{
//						printf("IEC61850: Value = 0x%X. id %d don't map to SCADA_ASDU id. Time = %d\n", pdu->value.ui, pdu->id, pdu->time_tag);
//					}
				}
//				else{
//					printf("IEC61850 error: id %d very big\n", pdu->id);
//				}
				// Next pdu
				pdu++;
				rdlen -= sizeof(data_unit);
			}

			// Send data to all registered SCADA_ASDUs
			if (psasdu->size){
				actscada = (SCADA_ASDU*) fscada.next;
				while(actscada){
					sedh->adr = actscada->pscada->ASDUaddr;
					psasdu->adr = actscada->pscada->ASDUaddr;
					mf_toendpoint(sendbuff, sizeof(ep_data_header) + sedh->len, actscada->pscada->ASDUaddr, DIRDN);
					printf("IEC61850: %d data_units sent to scada asdu=%d\n", psasdu->size, actscada->pscada->ASDUaddr);
					actscada = actscada->l.next;
				}
			}

			free(sendbuff);

			break;

		case EP_MSG_TIME_SYNC:
			tv.tv_sec  = *(time_t*)(buff + offset + sizeof(ep_data_header));
			tv.tv_usec = 0;

			res = settimeofday(&tv, NULL);

			printf("IEC61850: Time synchronization with master host %s. Address = %d.\n", res?"failed":"succeeded", edh->adr);

			break;
		}

		// move over the data
		offset += sizeof(ep_data_header);
		offset += edh->len;
	}

//#endif
	free(buff);

	return 0;
}

int asdu_parser(void){
SCADA_ASDU *actscada;
VIRT_ASDU *actasdu;
VIRT_ASDU_TYPE *actasdutype;
ASDU_DATAMAP *actasdudm;
LNODE *aln;
LNTYPE *alnt;
DOBJ *adobj;

	printf("ASDU: Start ASDU mapping to parse\n");

	// Create VIRT_ASDU_TYPE list
	actasdutype = (VIRT_ASDU_TYPE*) &fasdutype;
	alnt = (LNTYPE*) flntype.next;
	while(alnt){

		actasdutype = create_next_struct_in_list((LIST*) actasdutype, sizeof(VIRT_ASDU_TYPE));

		printf("ASDU: new VIRT_ASDU_TYPE\n");

		// Fill VIRT_ASDU_TYPE
		actasdutype->mylntype = alnt;

		// create ASDU_DATAMAP list
		actasdudm = (ASDU_DATAMAP*) &fdm;
		adobj = actasdutype->mylntype->lntype.pfdobj; // (DOBJ*) fdo.next;

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
						printf("ASDU: new SCADA_ASDU_DO for DOBJ name=%s type=%s: %d =>moveto=> %d by type=%d\n",
								adobj->dobj.name, adobj->dobj.type, actasdudm->meterid, actasdudm->scadaid, actasdudm->value_type);
					}else printf("ASDU: new SCADA_ASDU_DO for DOBJ error: Tag not found into mainmap.cfg");
			}else printf("ASDU: new SCADA_ASDU_DO for DOBJ (without mapping) name=%s type=%s\n", adobj->dobj.name, adobj->dobj.type);

			// Next DOBJ
			adobj = adobj->l.next;
		}
		if (fdm.next) actasdutype->fdmap = fdm.next;

		printf("ASDU: ready VIRT_ASDU_TYPE for LNTYPE id=%s \n", alnt->lntype.id);

		alnt = alnt->l.next;
	}


	// Create VIRT_ASDU and SCADA_ASDU lists
	actasdu = (VIRT_ASDU*) &fasdu;
	actscada = (SCADA_ASDU*) &fscada;
	aln = (LNODE*) fln.next;
	while(aln){
		if (aln->ln.options){
			actasdu = create_next_struct_in_list((LIST*) actasdu, sizeof(VIRT_ASDU));

			// Fill VIRT_ASDU
			actasdu->myln = aln;
			actasdu->ASDUaddr = atoi(aln->ln.options);
			actasdu->baseoffset = atoi(aln->ln.lninst) * IEC104_CHLENGHT;

			// If 'scada', create SCADA_ASDU
			if (strstr(actasdu->myln->ln.iedname, "scada")){
				actscada = create_next_struct_in_list((LIST*) actscada, sizeof(SCADA_ASDU));
				actscada->pscada = actasdu;
			}

			// Link to VIRT_ASDU_TYPE
			// Find LNTYPE.id = VIRT_ASDU_TYPE.LN.lntype
			actasdutype =  (VIRT_ASDU_TYPE*) fasdutype.next;
			while(actasdutype){
				alnt = actasdutype->mylntype;
				if (!strcmp(alnt->lntype.id, aln->ln.lntype)) break;
				actasdutype = actasdutype->l.next;
			}
			if (actasdutype){
				actasdu->myasdutype = actasdutype;
				printf("ASDU: VIRT_ASDU %s.%s.%s linked to TYPE %s\n",
						actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass, actasdutype->mylntype->lntype.id);
			}
			else printf("ASDU: VIRT_ASDU %s.%s.%s NOT linked to TYPE\n", actasdu->myln->ln.ldinst, actasdu->myln->ln.lninst, actasdu->myln->ln.lnclass);

			// Link ASDU_TYPE to DATAMAP

		}

		printf("ASDU: new VIRT_ASDU addr=%d for LN name=%s.%s.%s type=%s ied=%s\n",
				actasdu->ASDUaddr, aln->ln.ldinst, aln->ln.lninst, aln->ln.lnclass, aln->ln.lntype, aln->ln.iedname);

		aln = aln->l.next;
	}

	printf("ASDU: End ASDU mapping\n");

	return 0;
}

char mainapp[] 		= {"startiec"};
char unitlink[] 	= {"unitlink-iec104"};
char physlink[] 	= {"phy_tcp"};

struct config_device cd = {
		mainapp,
		unitlink,
		physlink,
		55555
};

// Load data to low level
void create_alldo(void){
VIRT_ASDU *sasdu = (VIRT_ASDU *) &fasdu;
ASDU_DATAMAP *pdm;
DOBJ	*pdo;
int adr;
frame_dobj fr_do;

	// Setup of unitlinks for getting DATA OBJECTS
		// get VIRT_ASDU => get LN_TYPE => get DATA_OBJECT list => write list to unitlink
		sasdu = ((VIRT_ASDU *) &fasdu)->l.next;
		while(sasdu){
			adr = atoi(sasdu->myln->ln.options);
			// find logical node type
			pdm = sasdu->myasdutype->fdmap;
			while (pdm){
				// write datatypes by sys msg EP_MSG_NEWDOBJ
				pdo = pdm->mydobj;
				fr_do.edh.adr = adr;
				fr_do.edh.len = sizeof(frame_dobj) - sizeof(ep_data_header);
				fr_do.edh.sys_msg = EP_MSG_NEWDOBJ;
				fr_do.id = pdm->meterid;
				strcpy(fr_do.name, pdo->dobj.name);

				// write to endpoint
				mf_toendpoint((char*) &fr_do, sizeof(frame_dobj), fr_do.edh.adr, DIRDN);
				usleep(5);	// delay for forming  next event

				pdm = pdm->l.next;
			}

			sasdu = sasdu->l.next;
		}
}

int virt_start(char *appname){
FILE *fmcfg;
int clen, ret;
struct stat fst;
pid_t chldpid;

VIRT_ASDU *sasdu = (VIRT_ASDU *) &fasdu;
char *p;

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
		cd.protoname = malloc(strlen(sasdu->myln->ln.ldinst) + 1);
		strcpy(cd.protoname, sasdu->myln->ln.ldinst);
		p = cd.protoname;
		while((*p != '.') && (*p)) p++;
		*p = 0;
		cd.phyname = p + 1;
		cd.name = appname;
		cd.addr = sasdu->ASDUaddr;

		// New endpoint
		mf_newendpoint(&cd, "/rw/mx00/unitlinks", 0);

		free(cd.protoname);
		sleep(1);	// Delay for forming next level endpoint

		sasdu = sasdu->l.next;
	};

	create_alldo();

	return ret;
}
