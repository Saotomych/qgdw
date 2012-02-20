/*
 * varcontrol.c
 *
 *  Created on: 27.01.2012
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "iec61850.h"
#include "multififo.h"
#include "asdu.h"
#include "varcontrol.h"

//static LIST fdefvt   = {NULL, NULL};		// first  varrec

//static LIST fvarbook = {NULL, NULL};		// first varbook for future
//static varbook *actvb;						// actual varbook for future

static int varrec_number;	// Argument counter for actual booking
static LIST fvarrec;
static varrec *lastvr;

static void* create_next_struct_in_list(LIST *plist, int size){
LIST *newlist;

	printf("Precreate list 0x%04X\n", plist);

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

static varrec* init_varrec(varrec *vr){
	if (!vr) return NULL;
	vr->name = malloc(sizeof(fcdarec));
	if (vr->name) memset(vr->name, 0, sizeof(fcdarec));
	else return NULL;
	vr->val = malloc(sizeof(value));
	if (vr->val) memset(vr->val, 0, sizeof(value));
	else{
		free(vr->name);
		return NULL;
	}
	return vr;
}

// Set callback function for variable changing events
void vc_setcallback(){

}

// Make memory allocation for all variables, read values and set pointers
void vc_init(pvalue vt, int len){
int i;
//varrec *defvt;		// actual varrec

	lastvr = (varrec*) &fvarrec;
	fvarrec.prev = NULL;
	fvarrec.next = NULL;

//	// Create const var table
//	defvt = (varrec*) &fdefvt;
//	for (i=0; i < len; i++){
//		defvt = create_next_struct_in_list((LIST*) &defvt->l, sizeof(varrec));
//		defvt->name = malloc(sizeof(fcdarec));
//		defvt->val = &vt[i];
//		defvt->name->fc = defvt->val->name;
//		defvt->prop = INTVAR | TRUEVALUE | vt->idtype;
//		defvt->time = 0;
//	}
//
//		// Bring to conformity with all internal variables and config variables
//		// It's equal constant booking
//
//	// Multififo init
//
//
}

static varrec* newappvarrec(value *val){
varrec *vr = create_next_struct_in_list(&(lastvr->l), sizeof(varrec));
	lastvr = vr;
	// Initialize varrec
	vr->name = malloc(sizeof(fcdarec));
	if (vr->name) memset(vr->name, 0, sizeof(fcdarec));
	vr->val = malloc(sizeof(value));
	if (vr->val) memcpy(vr->val, val, sizeof(value));
	vr->prop = INTVAR | TRUEVALUE | vr->val->idtype;;
	vr->iarg = varrec_number;
	varrec_number++;
	vr->time = 0;
	return vr;
}

static varrec* newiecvarrec(void){
varrec *vr = create_next_struct_in_list(&(lastvr->l), sizeof(varrec));
	// Initialize varrec
	if (init_varrec(vr)){	// mallocs for 'fcdarec' and 'value'
		lastvr = vr;
		vr->iarg = varrec_number;
		varrec_number++;
		vr->time = 0;
		// Copy global variable to local for local changes
		vr->val->idtype = STRING;
		vr->val->iddeftype = 0;
		vr->val->defval = 0;
		vr->val->idx = IECBASE;
		vr->prop = INTVAR | TRUEVALUE | vr->val->idtype;
		return vr;
	}else{
		if (vr){
			if (vr->val) free(vr->val);
			if (vr->name) free(vr->name);
			free(vr);
		}
		return NULL;
	}
}

// To book concrete variable by name
// Input data: name of variable
// Return pointer to value record and properties
varrec *vc_addvarrec(LNODE *actln, char *varname, value *defvr){
int i, x;
varrec *vr;
struct _IED *pied;
struct _LDEVICE *pld;
DOBJ *pdo;
ATTR *pda;	// for future
LNODE *pln = actln;
char *p, *po=0, *pa=0, *ptag=0;
char keywords[][10] = {
		{"APP:"},
		{"IED:"},
		{"LD:"},
		{"LN:"},
};

	pld = pln->ln.pmyld;
	pied = pln->ln.pmyied;

	for (i=0; i < sizeof(keywords)/10; i++){
		p = strstr(varname, keywords[i]);
		if (p == varname){
			switch(i){
			// if APP - Find pointer in table
			case 0: // APP:
					// Find value in default table
					x = 0;
					while(defvr[x].idx == x){
						if (!strcmp(defvr[x].name, varname)){
							// Register new value in varrec LIST
							vr = newappvarrec(&defvr[x]);
							if (vr){
								vr->name->fc = varname;
								vr->val->name = varname;
								return vr;
							}
							else{
								free(vr);
								lastvr->l.next = NULL;
								return NULL;
							}
						}
						x++;
					}

					break;

			// if IED, LD - Set const as text
			case 1:	// IED:
					if (pied){
						// Register new value in varrec LIST
						vr = newiecvarrec();
						if (vr){
							vr->name->fc = varname;
							vr->val->name = varname;
							// Value initialize
							vr->val->val = pied->desc;	// IED.desc as default
							vr->val->idx = IECBASE + IEDdesc;
							p = strstr(varname, "name"); // Find IED.name
							if (p){
								vr->val->idx = IECBASE + IEDname;
								vr->val->val = pied->name;
							}
							return vr;
						}else{
							lastvr->l.next = NULL;
							return NULL;
						}
					}

					break;

			case 2: // LD
					// Fill varrec as const of application
					if (pld){
						// Register new value in varrec LIST
						vr = newiecvarrec();
						if (vr){
							vr->name->fc = varname;
							vr->val->name = varname;
							// Value initialize
							// Set val if 'inst'
							vr->val->val = pld->inst;	// LD.inst as default
							vr->val->idx = IECBASE + LDinst;
							// Set val if 'desc'
							p = strstr(varname, "desc");
							if (p) vr->val->val = pld->desc;
							// Set val if 'name'. 'name' is part of desc and define ld's engineering name
							p = strstr(varname, "name");
							if (p){
								p = pld->desc;
								while((*p!='/') && (*p)) p++;
								vr->val->val = p;
							}
							return vr;
						}else{
							lastvr->l.next = NULL;
							return NULL;
						}
					}
					break;

			// if LN without DO/DA.name - Set const of field as text
			// if LN has DO.name - book this variable
			case 3: // LN

					// Find LN.field
// TODO Make find LN.field in future
//					inst="2"
//					lnClass="MMXU"
//					lnType="MMXUa"
//					prefix="M700"

				// Find all fields: po, pa, ptag
					p = strstr(varname, ":");
					if (p){
						// Set po to data object
						p++; po = p;
						p = strstr(p, ".");
						if (p){
							// Data object found
							// Set pa to data attribute as variant
							*p = 0;	p++; pa = p;
							p = strstr(p, ".");
							if (p){
								// Field for data attribute found
								*p = 0;	p++; ptag = p;
							}else{
								// This field is tag
								ptag = pa;
								pa = NULL;
							}
						}
					}else return NULL;

					if (actln){
						// Register new value in varrec LIST
						vr = newiecvarrec();
						if (vr){
							vr->name->fc = varname;
							vr->val->name = varname;
							// Value initialize
							// Find DOBJ as equal actual LN by type
							if (ptag){
								vr->val->idx = IECBASE + DOdesc;
								pdo = pln->ln.pmytype->pfdobj;
								while ((pdo) && (strcmp(pdo->dobj.name, po))) pdo = pdo->l.next;
								if (!pdo) return NULL;

		//						// find ATTR (in pa) for future IEC functions
		//						if (pa){
		//							pda = pdo->dobj.pmytype->pfattr;
		//							while((pda) && (strcmp(pa, pda->attr.name))) pda = pda->l.next;
		//							if (!pda) return NULL;
		//						}
		//
		//						if (!strcmp(ptag, "value")){
		//							// IF Value  =>  Book var in startiec and fill varrec as remote variable
		//							// TODO Variable booking
		//						}else{
		//							if (!strcmp(ptag, "desc")) vr->val->val = pdo->dobj.options;
		//							if (!strcmp(ptag, "name")) vr->val->val = pdo->dobj.name;
		//							if (!strcmp(ptag, "type")) vr->val->val = pdo->dobj.type;
		//						}
							}else{
								if (po){
									// IF const  =>  Fill varrec as const of application
									vr->val->val = actln->ln.lnclass;		// LN.class as default
									vr->val->idx = IECBASE + LNlnclass;
									if (!strcmp(po, "inst")){
										vr->val->val = actln->ln.lninst;
										vr->val->idx = IECBASE + LNlninst;
									}
									if (!strcmp(po, "type")){
										vr->val->val = actln->ln.lntype;
										vr->val->idx = IECBASE + LNlntype;
									}
									if (!strcmp(po, "prefix")){
										vr->val->val = actln->ln.prefix;
										vr->val->idx = IECBASE + LNprefix;
									}
									return vr;
								}
							}

							return vr;
						}else{
							lastvr->l.next = NULL;
							return NULL;
						}
					}

					break;
			}
		}
	}
	return NULL;
}

// To delete booking of concrete variable by name
int vc_destroyvarreclist(varrec *fvr){
varrec *vr = lastvr;
varrec *prevvr;

	while((vr->l.next != fvr) && (vr->l.prev)){

		// TODO Unsubscribe variable if needed

		// Free and switch to next varrec
		prevvr = vr->l.prev;
		if (vr->name) free(vr->name);
		if (vr->val) free(vr->val);
		free(vr);
		vr = prevvr;
		lastvr = vr;
	}

	vr->l.next = NULL;

	return 0;
}

void vc_checkvars(){

}
