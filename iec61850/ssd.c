/*
 * ssd.c
 *
 *  Created on: 13.07.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "iec61850.h"

struct _LNODETYPE *actlnodetype;
struct _DTYPE  *actdtype;

// Start points
LIST fied, fln, flntype, fdo, fdtype, fattr;
IED *flastied = (IED*) &fied;
LNODE *flastln = (LNODE*) &fln;
LNTYPE *flastlntype = (LNTYPE*) &flntype;
DOBJ *flastdo = (DOBJ*) &fdo;
DTYPE *flastdtype = (DTYPE*) &fdtype;
ATTR *flastattr = (ATTR*) &fattr;

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

// In: p - pointer to string
// Out: key - pointer to string with key of SCL file
//      par - pointer to string par for this key without ""
char* get_next_parameter(char *p, char **key, char **par){
int mode=0;

	*key = p;
	while (*p != '>'){
		if (!mode){
			// Key mode
			if (*p == '='){
				mode = 1;	// switch to par mode
				*p = 0;
			}
		}else{
			// Par mode
			if (*p == '"'){
				if (mode == 2){
					*p = 0;
					return p+1;		// key + par ready
				}
				if (mode == 1){
					mode = 2;
					*par = p+1;
				}
			}
		}
		p++;
	}

	return 0;
}

// *** Tag structure working ***//

void ssd_create_ied(const char *pTag){			// call parse ied
char *p;
char *key=0, *par=0;

	flastied = create_next_struct_in_list(&(flastied->l), sizeof(IED));

	// Parse parameters for ied
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flastied->ied.name = par;
			else
			if (strstr((char*) key, "inst")) flastied->ied.inst = par;
		}
	}while(p);

	printf("IEC: new IED: name=%s inst=%s\n", flastied->ied.name, flastied->ied.inst);

}

void ssd_create_ln(const char *pTag){			// call parse ln
char *p;
char *key=0, *par=0;
	flastln = create_next_struct_in_list(&(flastln->l), sizeof(LNODE));
	//DTYPE *adtype;
	//ATTR *dattr;
	//char *p;

	// Parse parameters for ln
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "lnclass")) flastln->ln.lnclass = par;
			else
			if (strstr((char*) key, "lninst")) flastln->ln.lninst = par;
			else
			if (strstr((char*) key, "iedname")) flastln->ln.iedname = par;
			else
			if (strstr((char*) key, "lntype")) flastln->ln.lntype = par;
			else
			if (strstr((char*) key, "ldinst")) flastln->ln.ldinst = par;
			else flastln->ln.options = par;
		}
	}while(p);

	printf("IEC: new LN: class=%s inst=%s iedname=%s ldclass=%s options=%s\n",
			flastln->ln.lnclass, flastln->ln.lninst, flastln->ln.iedname, flastln->ln.ldinst, flastln->ln.options);
}

void ssd_create_lntype(const char *pTag){			// call parse ln
char *p;
char *key=0, *par=0;
	flastlntype = create_next_struct_in_list(&(flastlntype->l), sizeof(LNTYPE));

	// Parse parameters for lntype
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastlntype->lntype.id = par;
			else
			if (strstr((char*) key, "lnclass")) flastlntype->lntype.lnclass = par;
		}
	}while(p);

	actlnodetype = &(flastlntype->lntype);

	flastlntype->lntype.maxdobj = 0;

	printf("IEC: new LNTYPE: id=%s lnclass=%s\n", flastlntype->lntype.id, flastlntype->lntype.lnclass);
}

void ssd_create_dobj(const char *pTag){			// call parse data
char *p;
char *key=0, *par=0;
	flastdo = create_next_struct_in_list(&(flastdo->l), sizeof(DOBJ));

	// Parse parameters for dobj
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flastdo->dobj.name = par;
			else
			if (strstr((char*) key, "type")) flastdo->dobj.type = par;
			else flastdo->dobj.options = par;
		}
	}while(p);

	flastdo->dobj.pmynodetype = actlnodetype;

	actlnodetype->maxdobj++;

	printf("IEC: new DATA OBJECT: name=%s type=%s options=%s\n", flastdo->dobj.name, flastdo->dobj.type, flastdo->dobj.options);
}

void ssd_create_dobjtype(const char *pTag){		// call parse data_type
char *p;
char *key=0, *par=0;
	flastdtype = create_next_struct_in_list(&(flastdtype->l), sizeof(DTYPE));

	// Parse parameters for dobjtype
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastdtype->dtype.id = par;
			else
			if (strstr((char*) key, "cdc")) flastdtype->dtype.cdc = par;
		}
	}while(p);

	actdtype = &(flastdtype->dtype);

	flastdtype->dtype.maxattr = 0;

	printf("IEC: new DATA OBJECT TYPE: id=%s cdc=%s\n", flastdtype->dtype.id, flastdtype->dtype.cdc);
}

void ssd_create_attr(const char *pTag){			// call parse attr
char *p;
char *key=0, *par=0;
	flastattr = create_next_struct_in_list(&(flastattr->l), sizeof(ATTR));

	// Parse parameters for attr
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flastattr->attr.name = par;
			else
			if (strstr((char*) key, "btype")) flastattr->attr.btype = par;
			else
			if (strstr((char*) key, "type")) flastattr->attr.type = par;
			else
			if (strstr((char*) key, "fc")) flastattr->attr.fc = par;
			else
			if (strstr((char*) key, "dchg")) flastattr->attr.dchg = par;
		}
	}while(p);

	flastattr->attr.pmydatatype = actdtype;

	actdtype->maxattr++;

	printf("IEC: new ATTRIBUTE: name=%s btype=%s type=%s fc=%s dchg=%s\n",
			flastattr->attr.name, flastattr->attr.btype, flastattr->attr.type, flastattr->attr.fc, flastattr->attr.dchg);
}

void ssd_create_enum(const char *pTag){			// call parse enum

}

void ssd_create_enumval(const char *pTag){			// call parse enum

}

void ssd_create_subst(const char *pTag){			// call parse substation

}

// *** End Tag structure working ***//


// Structures connecting function
void lnode2ied2types(void){
// find ied and type for every lnode
// ied by name-iedname
// type by id-lntype
LNODE *pln = (LNODE*) fln.next;
IED *pied;
LNTYPE *ptype;

	while(pln){
		// find ied
		pied = (IED*) fied.next;
		while(pied){
			if (!strcmp(pied->ied.name, pln->ln.iedname)) break;
			pied = pied->l.next;
		}
		if (pied){
			pln->ln.pmyied = &(pied->ied);
			printf("IEC61850: IED %s for LNODE %s.%s.%s.%s found\n", pln->ln.iedname, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst, pln->ln.options);
		}else{
			printf("IEC61850 error: IED %s for LNODE %s.%s.%s.%s not found\n", pln->ln.iedname, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst, pln->ln.options);
		}

		// find lntype
		ptype = (LNTYPE*) flntype.next;
		while(ptype){
			if (!strcmp(ptype->lntype.id, pln->ln.lntype)) break;
			ptype = ptype->l.next;
		}
		if (ptype){
			pln->ln.pmytype = &(ptype->lntype);
			printf("IEC61850: LNTYPE %s for LNODE %s.%s.%s.%s found\n", pln->ln.lntype, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst, pln->ln.options);
		}else{
			printf("IEC61850 error: LNTYPE %s for LNODE %s.%s.%s.%s not found\n", pln->ln.lntype, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst, pln->ln.options);
		}

		// Next LN
		pln = pln->l.next;
	}
}

void lnodetype2dobj(void){
// find 1th DOBJ for every LNTYPE
// dobj by pmynodetype - &(ptype->lntype)
LNTYPE *ptype = (LNTYPE*) flntype.next;
DOBJ *pdo;

	while(ptype){
		// find first DOBJ
		pdo = (DOBJ*) fdo.next;
		while(pdo){
			if (pdo->dobj.pmynodetype == &(ptype->lntype)) break;
			pdo = pdo->l.next;
		}

		if (pdo){
			ptype->lntype.pfdobj = pdo;
			printf("IEC61850: First DOBJ for LNTYPE %s.%s found\n", ptype->lntype.lnclass, ptype->lntype.id);
		}else{
			printf("IEC61850 error: First DOBJ for LNTYPE %s.%s not found\n", ptype->lntype.lnclass, ptype->lntype.id);
		}

		// next LNTYPE
		ptype = ptype->l.next;
	}
}

void dobj2dtype(void){
// find DTYPE for every DOBJ
// dtype by id - type
DOBJ *pdo = (DOBJ*) fdo.next;
DTYPE *pdt;

	while(pdo){
		pdt = fdtype.next;
		while(pdt){
			if (!strcmp(pdt->dtype.id, pdo->dobj.type)) break;
			pdt = pdt->l.next;
		}
		if (pdt){
			pdo->dobj.pmytype = &(pdt->dtype);
			printf("IEC61850: DTYPE %s for DOBJ %s found\n", pdt->dtype.id, pdo->dobj.name);
		}else{
			printf("IEC61850 error: DTYPE %s for DOBJ %s not found\n", pdt->dtype.id, pdo->dobj.name);
		}

		pdo = pdo->l.next;
	}
}

void dtype2attr(void){
// find 1th DA for DOType
// da by

}

void crossconnection(){
	// LNODE -> _IED & _LNODETYPE
	lnode2ied2types();
	// LNODETYPE -> DOBJ
	lnodetype2dobj();
	// _DOBJ -> _DTYPE &  _LNODETYPE
	dobj2dtype();
	// DTYPE -> ATTR
	dtype2attr();

}
