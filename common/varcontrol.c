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
#include "paths.h"
#include "ts_print.h"

static int varrec_number;	// Argument counter for actual attaching
static LIST fvarrec;
static varrec *lastvr;

// Pointer to full mapping config as text
char *MCFGfile;

extern LIST* create_next_struct_in_list(LIST *plist, int size);

// Get mapping parameters from special config file 'mainmap.cfg' of real ASDU frames from meters
int vc_get_map_by_name(char *name, uint32_t *mid){
char *p;

	*mid = 0;
	p = strstr(MCFGfile, name);
	if (!p) return -1;
	while((*p != 0xA) && (p != MCFGfile)) p--;
	while(*p <= '0') p++;
	*mid = atoi(p);

	return 0;
}

// Find DA with name and ptr equals to name and ptr DTYPE
// Return int equal string typedef
int vc_get_type_by_name(char *name, char *type){
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
					if (strstr(p1, "Struct")) ret = 1;
					else ret = -1;
				}
			}
			dattr = dattr->l.next;
		}
	}else ret = -1;

	return ret;
}

static varrec* init_varrec(varrec *vr, uint32_t vallen){
	if (!vr) return NULL;
	vr->name = malloc(sizeof(fcdarec));
	if (vr->name) memset(vr->name, 0, sizeof(fcdarec));
	else return NULL;
	vr->val = malloc(sizeof(value) * vallen);
	if (vr->val) memset(vr->val, 0, sizeof(value));
	else{
		free(vr->name);
		return NULL;
	}
	return vr;
}

static int get_type_idx(char *ptype){
	if (!strcmp(ptype, "VisString255")) return STRING;
	if (!strcmp(ptype, "INT32")) return INT32;
	if (!strcmp(ptype, "FLOAT32")) return FLOAT32;
	if (!strcmp(ptype, "Quality")) return QUALITY;
	if (!strcmp(ptype, "Timestamp")) return TIMESTAMP;
	return 0;
}

static size_t sizeof_idx(int idx){
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
int vc_init(){
FILE *fmcfg;
char *fname;
int clen, ret = 0;
struct stat fst;

	lastvr = (varrec*) &fvarrec;
	fvarrec.prev = NULL;
	fvarrec.next = NULL;

// Read mainmap.cfg into memory
	fname = malloc(strlen(getpath2configs()) + strlen("mainmap.cfg") + 1);
	strcpy(fname, getpath2configs());
	strcat(fname, "mainmap.cfg");

	if (stat(fname, &fst) == -1){
		ts_printf(STDOUT_FILENO, "IEC Virt: 'mainmap.cfg' file not found\n");
	}
	MCFGfile =  malloc(fst.st_size+1);
	fmcfg = fopen(fname, "r");
	clen = fread(MCFGfile, 1, (size_t) (fst.st_size), fmcfg);
	MCFGfile[fst.st_size] = 0; // make it null terminating string

	if (clen != fst.st_size) ret = -1;

	free(fname);

	return ret;
}

static varrec* newappvarrec(value *val){
varrec *vr = (varrec*) create_next_struct_in_list(&(lastvr->l), sizeof(varrec));
	lastvr = vr;
	// Initialize varrec
	vr->name = malloc(sizeof(fcdarec));
	if (vr->name) memset(vr->name, 0, sizeof(fcdarec));
	vr->val = malloc(sizeof(value));
	if (vr->val) memcpy(vr->val, val, sizeof(value));
	vr->maxval = 1;
	vr->prop = INTERNAL | TRUEVALUE;
	vr->time = 0;
	return vr;
}

static varrec* newiecvarrec(uint32_t vallen){
varrec *vr = (varrec*) create_next_struct_in_list(&(lastvr->l), sizeof(varrec));
	// Initialize varrec
	if (init_varrec(vr, vallen)){	// mallocs for 'fcdarec' and 'value'
		lastvr = vr;
		vr->time = 0;
		vr->maxval = vallen;
		// Copy global variable to local for local changes
		vr->val->idtype = STRING;
		vr->val->iddeftype = 0;
		vr->val->defval = 0;
		vr->val->idx = IECBASE;
		vr->prop = INTERNAL | TRUEVALUE;
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

char* vc_typetest(char *ptype){
	if (!strcmp(ptype, "VisString255")) return ptype;
	if (!strcmp(ptype, "INT32")) return ptype;
	if (!strcmp(ptype, "FLOAT32")) return ptype;
	if (!strcmp(ptype, "Quality")) return ptype;
	if (!strcmp(ptype, "Timestamp")) return ptype;
	return NULL;
}

// Add concrete variable of actln by name
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
uint32_t varlen = strlen(varname) + 1;

char keywords[][10] = {
		{"APP:"},
		{"IED:"},
		{"LD:"},
		{"LN:"},
		{"JR:"},
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
								vr->val->name = malloc(varlen);
								strcpy(vr->val->name, varname);
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
						vr = newiecvarrec(1);
						if (vr){
							vr->name->fc = varname;
							vr->name->ldinst = pld->inst;
							vr->name->lnClass = actln->ln.lnclass;
							vr->name->lnInst = actln->ln.lninst;
							vr->name->prefix = actln->ln.prefix;
							vr->name->doName = NULL;
							vr->name->daName = NULL;
							vr->asdu = 0;
							vr->id = 0;
							vr->val->name = malloc(varlen);
							strcpy(vr->val->name, varname);
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
						vr = newiecvarrec(1);
						if (vr){
							vr->name->fc = varname;
							vr->name->ldinst = pld->inst;
							vr->name->lnClass = actln->ln.lnclass;
							vr->name->lnInst = actln->ln.lninst;
							vr->name->prefix = actln->ln.prefix;
							vr->name->doName = NULL;
							vr->name->daName = NULL;
							vr->asdu = atoi(pld->inst);
							vr->id = 0;
							vr->val->name = malloc(varlen);
							strcpy(vr->val->name, varname);
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
			// if LN has DO.name - attach this variable
			case 3: // LN

					if (actln){
						// Register new value in varrec LIST
						vr = newiecvarrec(1);
						if (vr){

							vr->name->fc = varname;
							vr->name->ldinst = pld->inst;
							vr->name->lnClass = actln->ln.lnclass;
							vr->name->lnInst = actln->ln.lninst;
							vr->name->prefix = actln->ln.prefix;
							vr->asdu = atoi(actln->ln.pmyld->inst);
							vr->id = 0;
							vr->val->name = malloc(varlen);
							strcpy(vr->val->name, varname);
							vr->val->idx = IECBASE + DOdesc;

							// Find all fields: po, pa, ptag
							p = strstr(vr->val->name, ":");
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
							}
							vr->name->doName = po;
							vr->name->daName = pa;

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
							vr->val->val = vc_typetest(pdo->dobj.type);
							if (vr->val->val){
								// DO.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = IECBASE + IECVALUE;
								vr->val->idtype = get_type_idx(pdo->dobj.type);
								vr->val->val = malloc(sizeof_idx(vr->val->idtype));
								memset(vr->val->val, 0, sizeof_idx(vr->val->idtype));
								vr->prop = ATTACHING| NEEDFREE;
								vc_get_map_by_name(po, (uint32_t*) &(vr->id));
								// TODO Attach DO value

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
							vr->val->val = vc_typetest(pda->attr.btype);
							if (vr->val->val){
								// DA.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = IECBASE + IECVALUE;
								vr->val->idtype = get_type_idx(pda->attr.btype);
								vr->val->val = malloc(sizeof_idx(vr->val->idtype));
								memset(vr->val->val, 0, sizeof_idx(vr->val->idtype));
								vr->prop = ATTACHING | NEEDFREE;
								vc_get_map_by_name(po, (uint32_t*) &(vr->id));
								// TODO Attach DA value

								return vr;
							}

							// Find pba as BDA.name
							if (pda->attr.pmyattrtype) pbda = pda->attr.pmyattrtype->pfbattr;
							else pbda = NULL;
							while ((pbda) && (strcmp(pbda->battr.name, pba))) pbda = pbda->l.next;
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
							vr->val->val = vc_typetest(pbda->battr.btype);
							if (vr->val->val){
								// BDA.name is value with var type
								// Set vr->val->val to value
								vr->val->idx = IECBASE + IECVALUE;
								vr->val->idtype = get_type_idx(pbda->battr.btype);
								vr->val->val = malloc(sizeof_idx(vr->val->idtype));
								memset(vr->val->val, 0, sizeof_idx(vr->val->idtype));
								vr->prop = ATTACHING | NEEDFREE;
								vc_get_map_by_name(po, (uint32_t*) &(vr->id));
								// TODO Attach BDA value

								return vr;
							}

							return NULL;
						}else{
							lastvr->l.next = NULL;
							return NULL;
						}
					}

					break;
			case 4:		// JR
					// Get first position and value lenght
					po = strstr(p, "(");
					if (po == NULL) return NULL;
					po++;
					i = atoi(po);
					po = strstr(p, ",");
					if (po == NULL) return NULL;
					po++;
					x = atoi(po);

					// Find all fields: po, pa, ptag
					p = strstr(po, ":");
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
					}

					// Create varrec
					vr = newiecvarrec(x);
					if (vr){
						vr->name->doName = po;
						vr->name->daName = pa;
						vr->name->fc = varname;
						vr->name->ldinst = pld->inst;
						vr->name->lnClass = actln->ln.lnclass;
						vr->name->lnInst = actln->ln.lninst;
						vr->name->prefix = actln->ln.prefix;
						vr->asdu = atoi(actln->ln.pmyld->inst);
						vr->id = 0;

						// Find type of journal variable
						if (pln->ln.pmytype) pdo = pln->ln.pmytype->pfdobj;
							else pdo = NULL;
						while ((pdo) && (strcmp(pdo->dobj.name, po))) pdo = pdo->l.next;
						if (!pdo) return NULL;
						vr->val->val = vc_typetest(pdo->dobj.type);
						if (vr->val->val){
							po = pdo->dobj.type;
						}else{
							if (pdo->dobj.pmytype) pda = pdo->dobj.pmytype->pfattr;
							else pda = NULL;
							while ((pda) && (strcmp(pda->attr.name, pa))) pda = pda->l.next;
							if (!pda) return NULL;
							vr->val->val = vc_typetest(pda->attr.btype);
							if (vr->val->val){
								po = pda->attr.btype;
							}else{
								if (pda->attr.pmyattrtype) pbda = pda->attr.pmyattrtype->pfbattr;
								else pbda = NULL;
								while ((pbda) && (strcmp(pbda->battr.name, pba))) pbda = pbda->l.next;
								if (!pbda) return NULL;
								vr->val->val = vc_typetest(pbda->battr.btype);
								if (vr->val->val){
									po = pbda->battr.btype;
								}
							}
						}

						// Set all variables
						for (i = 0; i < x; i++){
							vr->val[i].name = malloc(varlen);
							strcpy(vr->val[i].name, varname);
							vr->val[i].idx = IECBASE + JRVALUE;
							vr->val[i].idtype = get_type_idx(po);
							vr->val[i].val = malloc(sizeof_idx(vr->val[i].idtype));
							memset(vr->val[i].val, 0, sizeof_idx(vr->val[i].idtype));
							vr->prop = ATTACHING | NEEDFREE;
						}

					}

					return vr;

					break;
			}
		}
	}
	return NULL;
}

void vc_freevarrec(varrec *vr){
varrec *prevvr = vr->l.prev;
int i;

	if (prevvr){
		prevvr->l.next = NULL;
		lastvr = prevvr;
	}

	free(vr->val->name);
	if (vr->prop & NEEDFREE){
		for (i = 0; i < vr->maxval; i++) free(vr->val[i].val);
	}
	if (vr->name) free(vr->name);
	if (vr->val) free(vr->val);
	free(vr);
}

// To delete attaching of concrete variable by name
int vc_destroyvarreclist(varrec *fvr){
varrec *vr = lastvr;
varrec *prevvr;

	if (!fvr) return 0;

	prevvr = vr->l.prev;
	vc_freevarrec(vr);
	while((vr != fvr) && (prevvr->l.prev)){
		// Set previuos vr
		vr = vr->l.prev;
		lastvr = vr;
		prevvr = vr->l.prev;
		// Free memory of value
		vc_freevarrec(vr);
	}

	vr->l.next = NULL;

	return 0;
}


// Make attach to all remote variables of last menu
void vc_attach_dataset(varrec *vr, time_t *t, uint32_t intr, LNODE *actln){
ep_data_header *edh;
varattach *vb;
char *varname;
u08 *varbuf;
uint32_t len;

	while(vr){
		// Need attach
		if (vr->prop & ATTACHING){
			// make full name LDinst.LNprefix.LNclass.LNdesc.
			// ready part = DOname.DAname.BDAname.JRNoffset

			len = sizeof(ep_data_header) + sizeof(varattach) + 7 + strlen(actln->ln.ldinst)
									  + strlen(actln->ln.prefix)
									  + strlen(actln->ln.lnclass)
									  + strlen(actln->ln.lninst)
									  + strlen(vr->name->fc);  // with start ep_data_header

			varbuf = malloc(len);
			edh = (ep_data_header*) varbuf;
			vb = (varattach*) ((char*) edh + sizeof(ep_data_header));
			varname = (char*) ((char*) vb + sizeof(varattach));
			ts_sprintf(varname, "%s/%s.%s.%s.%s", actln->ln.ldinst,
													actln->ln.prefix,
													actln->ln.lnclass,
													actln->ln.lninst,
													&(vr->name->fc)[3]);

			edh->adr = IDHMI;
			edh->sys_msg = EP_MSG_ATTACH;
			edh->len = len - sizeof(ep_data_header);
			edh->numep = 0;
			vb->lenname = strlen(varname);
			vb->time = *t;
			vb->id = atoi(actln->ln.ldinst);
			vb->uid = (uint32_t) vr;	// UID of variable is pointer to varrec
			vb->intr = intr;

			// Send attach this varrec
			if (len) mf_toendpoint((char*) varbuf, len, IDHMI, DIRDN);

			free(varbuf);
		}
		// Next varrec
		vr = vr->l.next;
	}
}

// Make unattach remote variables from end to vr->name->fc
void vc_unattach_dataset(varrec *fvr, LNODE *actln){
ep_data_header *edh;
varrec *vr;
uint32_t *uids;
u08 *varbuf;
uint32_t len = 0;

	vr = fvr;
	while(vr){
		len += sizeof(int);
		vr = vr->l.next;
	}

	varbuf = malloc(len + sizeof(ep_data_header));
	edh = (ep_data_header*) varbuf;
	edh->adr = IDHMI;
	edh->sys_msg = EP_MSG_UNATTACH;
	edh->len = len;
	edh->numep = 0;

	vr = fvr;
	uids = (uint32_t*)(varbuf + sizeof(ep_data_header));
	while(vr){
		*uids = vr->uid;
		uids++;
		vr = vr->l.next;
	}

	ts_printf(STDOUT_FILENO, "HMI: unattach\n");

	// Send unattach all varrec
	mf_toendpoint((char*) varbuf, len + sizeof(ep_data_header), IDHMI, DIRDN);

	free(varbuf);

}

varrec* vc_getfirst_varrec(void){

	return fvarrec.next;
}
