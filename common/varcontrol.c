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

static char* type_test(char *ptype){
	if (!strcmp(ptype, "VisString255")) return ptype;
	if (!strcmp(ptype, "INT32")) return ptype;
	if (!strcmp(ptype, "FLOAT32")) return ptype;
	if (!strcmp(ptype, "Quality")) return ptype;
	if (!strcmp(ptype, "Timestamp")) return ptype;
	return NULL;
}

static int get_type_idx(char *ptype){
	if (!strcmp(ptype, "VisString255")) return STRING;
	if (!strcmp(ptype, "INT32")) return INT32;
	if (!strcmp(ptype, "FLOAT32")) return FLOAT32;
	if (!strcmp(ptype, "Quality")) return QUALITY;
	if (!strcmp(ptype, "Timestamp")) return TIMESTAMP;
	return 0;
}

static int sizeof_idx(int idx){
	if (idx == STRING) return 256;
	if (idx == INT32) return 4;
	if (idx == FLOAT32) return 4;
	if (idx == QUALITY) return 4;
	if (idx == TIMESTAMP) return sizeof(time_t);
	return 0;
}

// Set callback function for variable changing events
void vc_setcallback(){

}

// Make memory allocation for all variables, read values and set pointers
void vc_init(pvalue vt, int len){

	lastvr = (varrec*) &fvarrec;
	fvarrec.prev = NULL;
	fvarrec.next = NULL;

	// TODO Multififo init


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
ATTR *pda;
BATTR *pbda;
LNODE *pln = actln;
char *p, *po=0, *pa=0, *pba=0;
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

				// Find all fields: po, pa, ptag
					p = strstr(varname, ":");
					if (p){
						// Set po to data object tag
						p++; po = p;
						p = strstr(p, ".");
						if (p){
							// Data object found
							// Set pa to data attribute as variant
							*p = 0;	p++; pa = p;
							p = strstr(p, ".");
							if (p){
								// Field for data attribute found
								*p = 0;	p++; pba = p;
								// Find next '.'
								p = strstr(p, ".");
								if (p) *p = 0;
							}
						}
					}else return NULL;

					if (actln){
						// Register new value in varrec LIST
						vr = newiecvarrec();
						if (vr){

							vr->name->fc = varname;
							vr->val->name = varname;
							vr->val->idx = IECBASE + DOdesc;

							// Find po as DO.name
							if (pln->ln.pmytype) pdo = pln->ln.pmytype->pfdobj;
							else pdo = NULL;
							while ((pdo) && (strcmp(pdo->dobj.name, po))) pdo = pdo->l.next;

							if (!pdo){
								// po is LN.<field>
								if (!strcmp(po, "class")){
									vr->val->val = actln->ln.lnclass;
									vr->val->idx = IECBASE + LNlnclass;
									return vr;
								}
								if (!strcmp(po, "inst")){
									vr->val->val = actln->ln.lninst;
									vr->val->idx = IECBASE + LNlninst;
									return vr;
								}
								if (!strcmp(po, "type")){
									vr->val->val = actln->ln.lntype;
									vr->val->idx = IECBASE + LNlntype;
									return vr;
								}
								if (!strcmp(po, "prefix")){
									vr->val->val = actln->ln.prefix;
									vr->val->idx = IECBASE + LNprefix;
									return vr;
								}
								return NULL;
							}

							// po is DO.name
							vr->val->val = type_test(pdo->dobj.type);
							if (vr->val->val){
								// DO.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = get_type_idx(vr->val->val);
								vr->val->val = malloc(sizeof_idx(vr->val->idx));
								return vr;
							}

							// Find pa as DA.name
							if (pdo->dobj.pmytype) pda = pdo->dobj.pmytype->pfattr;
							else pda = NULL;
							while ((pda) && (strcmp(pda->attr.name, pa))) pda = pda->l.next;
							if (!pda){
								// pa is DO.<field>
								if (!strcmp(pa, "desc")){
									vr->val->val = pdo->dobj.options;
									vr->val->idx = IECBASE + DOdesc;
									return vr;
								}
								if (!strcmp(pa, "name")){
									vr->val->val = pdo->dobj.name;
									vr->val->idx = IECBASE + DOname;
									return vr;
								}
								if (!strcmp(pa, "type")){
									vr->val->val = pdo->dobj.type;
									vr->val->idx = IECBASE + DOtype;
									return vr;
								}
								return NULL;
							}

							// pa is DA.name
							vr->val->val = type_test(pda->attr.btype);
							if (vr->val->val){
								// DA.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = get_type_idx(vr->val->val);
								vr->val->val = malloc(sizeof_idx(vr->val->idx));
								return vr;
							}

							// Find pba as BDA.name
							if (pda->attr.pmyattrtype) pbda = pda->attr.pmyattrtype->pfbattr;
							else pbda = NULL;
							while ((pbda) && (strcmp(pbda->battr.name, pa))) pbda = pbda->l.next;
							if (!pbda){
								// pbda is DA.<field>
								if (!strstr(pba, "name")){
									vr->val->val = pda->attr.name;
									vr->val->idx = IECBASE + DAname;
									return vr;
								}
								if (!strstr(pba, "bType")){
									vr->val->val = pda->attr.btype;
									vr->val->idx = IECBASE + DAbtype;
									return vr;
								}
								if (!strstr(pba, "type")){
									vr->val->val = pda->attr.type;
									vr->val->idx = IECBASE + DAtype;
									return vr;
								}
								if (!strstr(pba, "fc")){
									vr->val->val = pda->attr.fc;
									vr->val->idx = IECBASE + DAfc;
									return vr;
								}
								if (!strstr(pba, "dchg")){
									vr->val->val = pda->attr.dchg;
									vr->val->idx = IECBASE + DAdchg;
									return vr;
								}
								if (!strstr(pba, "dupd")){
									vr->val->val = pda->attr.dupd;
									vr->val->idx = IECBASE + DAdupd;
									return vr;
								}
								if (!strstr(pba, "qchg")){
									vr->val->val = pda->attr.qchg;
									vr->val->idx = IECBASE + DAqchg;
									return vr;
								}

								return NULL;
							}

							// pba is BDA.name
							vr->val->val = type_test(pbda->battr.btype);
							if (vr->val->val){
								// BDA.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = get_type_idx(vr->val->val);
								vr->val->val = malloc(sizeof_idx(vr->val->idx));
								return vr;
							}

							return NULL;
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
		varrec_number--;
	}

	vr->l.next = NULL;

	return 0;
}

void vc_checkvars(){

}
