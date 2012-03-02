/*
 * cid.c
 *
 *  Created on: 13.07.2011
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "iec61850.h"
#include "xml.h"

struct _IED *actlied = NULL;
struct _LDEVICE *actlldevice = NULL;
struct _LNODETYPE *actlnodetype = NULL;
struct _DTYPE  *actdtype = NULL;
struct _ATYPE  *actatype = NULL;

// Start points for IEC61850
LIST fied, fld, fln, flntype, fdo, fdtype, fattr, fatype, fbattr;
IED *flastied = (IED*) &fied;
LDEVICE *flastld = (LDEVICE*) &fld;
LNODE *flastln = (LNODE*) &fln;
LNTYPE *flastlntype = (LNTYPE*) &flntype;
DOBJ *flastdo = (DOBJ*) &fdo;
DTYPE *flastdtype = (DTYPE*) &fdtype;
ATTR *flastattr = (ATTR*) &fattr;
ATYPE *flastatype = (ATYPE*) &fatype;
BATTR *flastbattr = (BATTR*) &fbattr;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *new;
	plist->next = malloc(size);

	if (!plist->next){
		printf("IEC61850: malloc error:%d - %s\n",errno, strerror(errno));
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
static char* get_next_parameter(char *p, char **key, char **par){
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

void cid_create_ied(const char *pTag){			// call parse ied
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
			if (strstr((char*) key, "desc")) flastied->ied.desc = par;
		}
	}while(p);

	actlied = &(flastied->ied);

	printf("IEC61850: new IED: name=%s desc=%s\n", flastied->ied.name, flastied->ied.desc);
}

void cid_create_ld(const char *pTag){			// call parse ld
char *p;
char *key=0, *par=0;
	flastld = create_next_struct_in_list(&(flastld->l), sizeof(LDEVICE));
	// Parse parameters for ldevice
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "inst")) flastld->ld.inst = par;
			else
			if (strstr((char*) key, "desc")) flastld->ld.desc = par;
		}
	}while(p);

	flastld->ld.pmyied = actlied;

	actlldevice = &(flastld->ld);

	printf("IEC61850: new LDevice: inst=%s desc=%s\n", flastld->ld.inst, flastld->ld.desc);
}

void cid_create_ln(const char *pTag){			// call parse ln
char *p;
char *key=0, *par=0;
	flastln = create_next_struct_in_list(&(flastln->l), sizeof(LNODE));

	// Parse parameters for ln
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "lnClass")) flastln->ln.lnclass = par;
			else
			if (strstr((char*) key, "lnInst") || strstr((char*) key, "inst")) flastln->ln.lninst = par;
			else
			if (strstr((char*) key, "iedName")) flastln->ln.iedname = par;
			else
			if (strstr((char*) key, "ldInst")) flastln->ln.ldinst = par;
			else
			if (strstr((char*) key, "lnType")) flastln->ln.lntype = par;
			else
			if (strstr((char*) key, "prefix")) flastln->ln.prefix = par;
		}
	}while(p);

	flastln->ln.pmyld = actlldevice;

	if(actlldevice) flastln->ln.ldinst = actlldevice->inst;

	flastln->ln.pmyied = actlied;

	if(actlied) flastln->ln.iedname = actlied->name;

	printf("IEC61850: new LN: class=%s inst=%s iedname=%s ldinst=%s\n",
			flastln->ln.lnclass, flastln->ln.lninst, flastln->ln.iedname, flastln->ln.ldinst);
}

void cid_create_lntype(const char *pTag){			// call parse ln
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
			if (strstr((char*) key, "lnClass")) flastlntype->lntype.lnclass = par;
		}
	}while(p);

	actlnodetype = &(flastlntype->lntype);

	flastlntype->lntype.maxdobj = 0;

	printf("IEC61850: new LNTYPE: id=%s lnclass=%s\n", flastlntype->lntype.id, flastlntype->lntype.lnclass);
}

void cid_create_dobj(const char *pTag){			// call parse data
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
			else
			if (strstr((char*) key, "desc")) flastdo->dobj.options = par;
		}
	}while(p);

	flastdo->dobj.pmynodetype = actlnodetype;

	actlnodetype->maxdobj++;

	printf("IEC61850: new DATA OBJECT: name=%s type=%s options=%s\n", flastdo->dobj.name, flastdo->dobj.type, flastdo->dobj.options);
}

void cid_create_dobjtype(const char *pTag){		// call parse data_type
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

	printf("IEC61850: new DATA OBJECT TYPE: id=%s cdc=%s\n", flastdtype->dtype.id, flastdtype->dtype.cdc);
}

void cid_create_attr(const char *pTag){			// call parse attr
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
			if (strstr((char*) key, "bType")) flastattr->attr.btype = par;
			else
			if (strstr((char*) key, "type")) flastattr->attr.type = par;
			else
			if (strstr((char*) key, "fc")) flastattr->attr.fc = par;
			else
			if (strstr((char*) key, "dchg")) flastattr->attr.dchg = par;
			else
			if (strstr((char*) key, "dupd")) flastattr->attr.dupd = par;
			else
			if (strstr((char*) key, "qchg")) flastattr->attr.qchg = par;
		}
	}while(p);

	flastattr->attr.pmydatatype = actdtype;

	actdtype->maxattr++;

	printf("IEC61850: new ATTRIBUTE: name=%s btype=%s type=%s fc=%s dchg=%s\n",
			flastattr->attr.name, flastattr->attr.btype, flastattr->attr.type, flastattr->attr.fc, flastattr->attr.dchg);
}

void cid_create_attrtype(const char *pTag){			// call parse attrtype
char *p;
char *key=0, *par=0;
	flastatype = create_next_struct_in_list(&(flastatype->l), sizeof(ATYPE));

	// Parse parameters for dobjtype
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "id")) flastatype->atype.id = par;
		}
	}while(p);

	actatype = &(flastatype->atype);

	flastatype->atype.maxbattr = 0;

	printf("IEC61850: new ATTRIBUTE TYPE: id=%s \n", flastatype->atype.id);
}

void cid_create_bda(const char *pTag){			// call parse baseattrtype
char *p;
char *key=0, *par=0;
	flastbattr = create_next_struct_in_list(&(flastbattr->l), sizeof(BATTR));

	// Parse parameters for attr
	p = (char*) pTag;
	do{
		p = get_next_parameter(p, &key, &par);
		if (p){
			if (strstr((char*) key, "name")) flastbattr->battr.name = par;
			else
			if (strstr((char*) key, "bType")) flastbattr->battr.btype = par;
		}
	}while(p);

	flastbattr->battr.pmyattrtype = actatype;

	actatype->maxbattr++;

	printf("IEC61850: new BASE ATTRIBUTE: name=%s btype=%s\n",
				flastbattr->battr.name, flastbattr->battr.btype);

}

void cid_create_enum(const char *pTag){			// call parse enum

}

void cid_create_enumval(const char *pTag){			// call parse enum

}

void cid_create_subst(const char *pTag){			// call parse substation

}

// *** End Tag structure working ***//


// Structures connecting function
void lnode2ied2ldev2types(void){
// find ied, ldev and type for every lnode
// ied by name-iedname
// ldev by inst-ldinst
// type by id-lntype
LNODE *pln = (LNODE*) fln.next;
IED *pied;
LDEVICE *pld;
LNTYPE *ptype;

	while(pln){
		// find ldevice
		pld = (LDEVICE*) fld.next;
		while((pld) && (strcmp(pld->ld.inst, pln->ln.ldinst))) pld = pld->l.next;
		// ldevice found ?
		if (pld){
			pln->ln.pmyld = &(pld->ld);
			printf("IEC61850: LDEVICE %s for LNODE %s.%s.%s found\n", pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
		}else{
			printf("IEC61850 error: LDEVICE %s for LNODE %s.%s.%s not found\n", pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
		}

		// find ied
		pied = (IED*) fied.next;
		while((pied) && (strcmp(pied->ied.name, pln->ln.iedname))) pied = pied->l.next;
		// ied found ?
		if (pied){
			// LNODE -> _IED
			pln->ln.pmyied = &(pied->ied);
			// LDEVICE -> _IED
			if(pln->ln.pmyld) pln->ln.pmyld->pmyied = pln->ln.pmyied;
			printf("IEC61850: IED %s for LNODE %s.%s.%s found\n", pln->ln.iedname, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
		}else{
			printf("IEC61850 error: IED %s for LNODE %s.%s.%s not found\n", pln->ln.iedname, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
		}

		// find lntype
		ptype = (LNTYPE*) flntype.next;
		while((ptype) && (strcmp(ptype->lntype.id, pln->ln.lntype))) ptype = ptype->l.next;
		// lntype found ?
		if (ptype){
			pln->ln.pmytype = &(ptype->lntype);
			printf("IEC61850: LNTYPE %s for LNODE %s.%s.%s found\n", pln->ln.lntype, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
		}else{
			printf("IEC61850 error: LNTYPE %s for LNODE %s.%s.%s not found\n", pln->ln.lntype, pln->ln.lnclass, pln->ln.lninst, pln->ln.ldinst);
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
		while((pdo) && (pdo->dobj.pmynodetype != &(ptype->lntype)))	pdo = pdo->l.next;
		// pdo found ?
		if (pdo){
			ptype->lntype.pfdobj = pdo;
			printf("IEC61850: First DOBJ %s for LNTYPE %s.%s found\n", pdo->dobj.name, ptype->lntype.lnclass, ptype->lntype.id);
		}else{
			printf("IEC61850 error: First DOBJ for LNTYPE %s.%s not found\n", ptype->lntype.lnclass, ptype->lntype.id);
		}

		// TODO Create iec61850 set of dataobjects for each LN

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
		// find dtype
		pdt = fdtype.next;
		while((pdt) && (strcmp(pdt->dtype.id, pdo->dobj.type))) pdt = pdt->l.next;
		// pdt found ?
		if (pdt){
			pdo->dobj.pmytype = &(pdt->dtype);
			printf("IEC61850: DTYPE %s for DOBJ %s found\n", pdt->dtype.id, pdo->dobj.name);
		}else{
			printf("IEC61850 error: DTYPE for DOBJ %s not found\n", pdo->dobj.name);
		}

		pdo = pdo->l.next;
	}
}

void dtype2attr(void){
// find 1th DA for DOType
// da by pmydatatype - &(pdo->dtype)
DTYPE *pdt = (DTYPE*) fdtype.next;
ATTR *pa;

	while(pdt){
		// find 1th attr
		pa = (ATTR*) fattr.next;
		while((pa) && (pa->attr.pmydatatype != &(pdt->dtype))) pa = pa->l.next;
		// pa found ?
		if (pa){
			pdt->dtype.pfattr = pa;
			printf("IEC61850: First ATTR %s(%s) for DTYPE %s found\n", pa->attr.name, pa->attr.btype, pdt->dtype.id);
		}else{
			printf("IEC61850 error: ATTR for DTYPE %s not found\n", pdt->dtype.id);
		}

		pdt = pdt->l.next;
	}
}

void atype2bda(void){
// find 1th BDA for DAType
ATYPE *pat = (ATYPE*) fatype.next;
BATTR *pba;

	while (pat){
		// find 1th base attr
		pba = (BATTR*) fbattr.next;
		while ((pba) && (pba->battr.pmyattrtype != &(pat->atype))) pba = pba->l.next;
		// pba found ?
		if (pba){
			pat->atype.pfbattr = pba;
			printf("IEC61850: First BASE ATTR %s(%s) for ATYPE %s found\n", pba->battr.name, pba->battr.btype, pat->atype.id);
		}else{
			printf("IEC61850 error: BASE ATTR for ATYPE %s not found\n", pat->atype.id);
		}

		pat = pat->l.next;
	}
}

void attr2atype(void){
// find DTYPE for every DOBJ
// dtype by id - type
ATTR *pda = (ATTR*) fattr.next;
ATYPE *pat;

	while(pda){
		// find atype
		pat = fatype.next;
		while((pat) && (pda->attr.type) && (strcmp(pat->atype.id, pda->attr.type))) pat = pat->l.next;
		// pat found ?
		if (pat){
			pda->attr.pmyattrtype = &(pat->atype);
			printf("IEC61850: DTYPE %s for DOBJ %s found\n", pat->atype.id, pda->attr.name);
		}else{
			printf("IEC61850 error: DTYPE for DOBJ %s not found\n", pda->attr.name);
		}

		pda = pda->l.next;
	}
}

void crossconnection(){
	// LNODE -> _IED & _LDEVICE & _LNODETYPE
	lnode2ied2ldev2types();
	// LNODETYPE -> DOBJ
	lnodetype2dobj();
	// _DOBJ -> _DTYPE &  _LNODETYPE
	dobj2dtype();
	// DTYPE -> ATTR
	dtype2attr();
	// ATTRTYPE -> BDA
	atype2bda();
	// _ATTR -> _ATYPE
	attr2atype();
}

// Create structures according to ieclevel.cid
int cid_build(char *filename){
char *SCLfile;
FILE *fcid;
int cidlen, ret = 0;
struct stat fst;
 	// Get size of main config file
	if (stat(filename, &fst) == -1){
		printf("IEC61850: SCL file not found\n");
	}

	SCLfile = malloc(fst.st_size);

	// Loading main config file
	fcid = fopen(filename, "r");
	cidlen = fread(SCLfile, 1, (size_t) (fst.st_size), fcid);
	if(!strstr(SCLfile, "</SCL>"))
	{
		printf("IEC61850: SCL is incomplete\n");
		exit(1);
	}
	if (cidlen == fst.st_size) XMLSelectSource(SCLfile);
	else ret = -1;

	return ret;
}
