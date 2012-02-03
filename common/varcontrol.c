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

static LIST fdefvt   = {NULL, NULL};		// first  varrec

static LIST fvarbook = {NULL, NULL};		// first varbook
static varbook *actvb;		// actual varbook

static varrec* create_varrec(void){
varrec *vr;
	vr = malloc(sizeof(varrec));
	vr->name = malloc(sizeof(fcdarec));
	vr->val = malloc(sizeof(value));
	return vr;
}

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

// Set callback function for variable changing events
void vc_setcallback(){

}

// Make memory allocation for all variables, read values and set pointers
void vc_init(pvalue vt, int len){
int i;
varrec *defvt;		// actual varrec

	// Create const var table
	defvt = (varrec*) &fdefvt;
	for (i=0; i < len; i++){
		defvt = create_next_struct_in_list((LIST*) &defvt->l, sizeof(varrec));
		defvt->name = malloc(sizeof(fcdarec));
		defvt->val = &vt[i];
		defvt->name->fc = defvt->val->name;
		defvt->prop = INTVAR | TRUEVALUE;
		defvt->time = 0;
	}

		// Bring to conformity with all internal variables and config variables
		// It's equal constant booking

	// Multififo init


}

// To book concrete variable by name
// Input data: name of variable
// Return pointer to value record and properties
varrec *vc_addvarrec(char *varname, LNODE *actln){
varrec *vr;
struct _IED *pied;
struct _LDEVICE *pld;
DOBJ *pdo;
LNODE *pln = actln;
char *p, i;
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
					vr = (varrec*) fdefvt.next;
					while(vr){
						if (!strcmp(vr->name->fc, varname)) return vr;
						vr = vr->l.next;
					}
					break;

			// if IED, LD - Set const as text
			case 1:	// IED:
					if (!pied) return NULL;
					vr = create_varrec();
					vr->name->fc = varname;
					p = strstr(varname, "name");
					if (p) vr->val->val = pied->name;
					// Set val if 'desc'
					p = strstr(varname, "desc");
					if (p) vr->val->val = pied->desc;
					return vr;
					break;

			case 2: // LD
					// Fill varrec as const of application
					if (!pld) return NULL;
					vr = create_varrec();
					vr->name->fc = varname;

					// Set val if 'inst'
					p = strstr(varname, "inst");
					if (p) vr->val->val = pld->inst;
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

					// Find DOBJ
					pdo = pln->ln.pmytype->pfdobj;
					p = strstr(varname, ".");
					if (p){
						p++;
						while(pdo){
							if (!strcmp(pdo->dobj.name, p)){
								// TODO find pda for future IEC functions
								break;
							}
							pdo = pdo->l.next;
						}
					}else return NULL;
					if (!pdo) return NULL;

					// Create new varrec
					vr = create_varrec();
					vr->name->fc = varname;

					// Value or config const

					// IF Value  =>  Book var in startiec and fill varrec as remote variable

					// IF const  =>  Fill varrec as const of application

					break;

			}
		}
	}




	return NULL;
}

// To delete booking of concrete variable by name
int vc_rmvarrec(char *varname){

	return 0;
}

void vc_checkvars(){

}
