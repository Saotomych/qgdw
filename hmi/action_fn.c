/*
 * action_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "hmi.h"

extern LNODE *actlnode;

// Values for change visible lnode types
// For indication
static char lntypes[][50] = {
		{"Телеизмерения"},
		{"Телесигнализация"},
		{"Телеуправление"},
};
// For filter
static char lnclasses[][5] = {
		{"MMXU"},
		{"MSQI"},
		{"MMTR"},
};

static void refreshvars(menu *actmenu){
int i, idx, j;
	for (i = 0; i < actmenu->count_item; i++){
		if (actmenu->pitems[i]->vr){
			if (actmenu->pitems[i]->vr->val->idx & IECBASE){
				idx = actmenu->pitems[i]->vr->val->idx - IECBASE;
				if (idx == IEDdesc) actmenu->pitems[i]->vr->val->val = (char*) actlnode->ln.pmyied->desc;
				if (idx == IEDname) actmenu->pitems[i]->vr->val->val = actlnode->ln.pmyied->name;
				if (idx == LDinst) actmenu->pitems[i]->vr->val->val = actlnode->ln.pmyld->inst;
				if (idx == LDname) actmenu->pitems[i]->vr->val->val = strchr(actlnode->ln.pmyld->inst, '/') + 1;
				if (idx == LDdesc) actmenu->pitems[i]->vr->val->val = actlnode->ln.pmyld->desc;
				if (idx == LNlnclass) actmenu->pitems[i]->vr->val->val = actlnode->ln.lnclass;
				if (idx == LNlninst) actmenu->pitems[i]->vr->val->val = actlnode->ln.lninst;
				if (idx == LNlntype) actmenu->pitems[i]->vr->val->val = actlnode->ln.lntype;
				if (idx == LNprefix) actmenu->pitems[i]->vr->val->val = actlnode->ln.prefix;
			}else{
				idx = actmenu->pitems[i]->vr->val->idx;
				// Set text of type LN
				if (idx == 27){
					// Type text change
					for (j = 0; j < 3; j++){
						if (!strcmp(lnclasses[j], actlnode->ln.lnclass)) break;
					}
					*((int*)(actmenu->pitems[i]->vr->val->val)) = lntypes[j];
				}
			}
		}
	}
}

// Var parser: converts indexes of arguments to indexes of defvalue variables
// Input: pointer to varrec for begin initialize actvr, ptr to array of defvalue indexes for vars, lenght of array
// Return pointer to value, NULL for end of list
varrec* get_argindex(varrec *vr, int *pidx, int len){
static varrec *actvr;
int l;

	// Initialize actvr
	if (vr){
		actvr = vr;
		return actvr;
	}

	if (!actvr) return NULL;

	while(actvr){
		l = len;
		while(l){
			l--;
			if (pidx[l] == actvr->val->idx){
				actvr = actvr->l.next;
				return actvr;
			}
		}
		actvr = actvr->l.next;
	}

	return NULL;
}

//---*** Set of action functions ***---//
// Function change pointer of LNODE to pointer of next LNODE with equal class by filter
int next_ln(void *arg){
LNODE **pbln = (LNODE**) &actlnode;
LNODE *pln = *pbln;
char *filter = pln->ln.lnclass;

	// Function body
    if (pln->l.next){
		do{
			pln = pln->l.next;
			if (!strcmp(pln->ln.lnclass, filter)){
				*pbln = pln;
				return 0;
			}
		}while (pln->l.next);
	}

	return 1;
}

// Function change pointer (arg[0]) to pointer of previous LNODE with equal class
int prev_ln(void *arg){
LNODE **pbln = (LNODE**) &actlnode;
LNODE *pln = *pbln;
char *filter = pln->ln.lnclass;

	// Function body
	if (pln->l.prev){
		do{
			if (pln->l.prev == &fln) break;
			pln = pln->l.prev;
			if (!strcmp(pln->ln.lnclass, filter)){
				*pbln = pln;
				return 0;
			}
		}while (pln->l.prev);
	}

	return 0;
}

// Function change pointer (arg[0]) to pointer of first LNODE with next class in array of classes
int prev_type_ln(void *arg){
LNODE **pbln =  (LNODE**) &actlnode;
LNODE *pln = *pbln;
char *lntype;
int i;

	for (i = 0; i < 3; i++){
		if (!strcmp(lnclasses[i], pln->ln.lnclass)) break;
	}
	if (i) i--;
	else i = 2;
	lntype = lnclasses[i];

	pln = (LNODE*) fln.next;
	while ((strcmp(lntype, pln->ln.lnclass)) && (pln)) pln = pln->l.next;
	if (pln) *pbln = pln;

	return REMAKEMENU + i;
}

// Function change pointer (arg[0]) to pointer of first LNODE with previous class in array of classes
int next_type_ln(void *arg){
LNODE **pbln =  (LNODE**) &actlnode;
LNODE *pln = *pbln;
char *lntype;
int i;

	for (i = 0; i < 3; i++){
		if (!strcmp(lnclasses[i], pln->ln.lnclass)) break;
	}
	i++;
	if (i >= 3) i = 0;
	lntype = lnclasses[i];

	pln = (LNODE*) fln.next;
	while ((strcmp(lntype, pln->ln.lnclass)) && (pln)) pln = pln->l.next;
	if (pln) *pbln = pln;

//	destroy_menu(DIR_SIDEBKW);
//	num_menu = create_menu(lnmenunames[i]);

	return REMAKEMENU + i;
}

// Array of structures "synonym to function"
fact actfactset[] = {
		{"changeln", (void*) prev_ln, NULL},      			// for left
		{"changeln", (void*) next_ln, NULL},					// for right
		{"changetypeln", (void*) prev_type_ln, NULL},			// for left
		{"changetypeln", (void*) next_type_ln, NULL},			// for right
};

//---*** External IP ***---//
int call_action(int direct, menu *actmenu){
int ret = 0;
int i;
char *act = actmenu->pitems[actmenu->num_item]->action;

	get_argindex((varrec*) actmenu->fvarrec.next, 0, 0);	// Set first varrec to var parser

	// Separate keys and call action
	switch(direct){
	case 0xf800:
				 for (i=0; i < sizeof(actfactset) / sizeof(fact); i+=2){
					if (!strcmp(actfactset[i].action, act)){
						ret = actfactset[i].proloque(actmenu);
					}
				 }
				 break;
	case 0xf801:
				 for (i=1; i < (sizeof(actfactset) / sizeof(fact)); i+=2){
					 if (!strcmp(actfactset[i].action, act)){
						 ret = actfactset[i].proloque(actmenu);
					 }
				 }
				 break;
	}

	refreshvars(actmenu);

	return ret;
}

LNODE* setdef_lnode(int lnclass, menu *actmenu){
LNODE *ln = (LNODE*) &fln.next;
	while (ln){
		if (ln->ln.lnclass){
			if (!strcmp(ln->ln.lnclass, lnclasses[lnclass])){
				actlnode = ln;
				if (actmenu) refreshvars(actmenu);
				return ln;
			}
		}
		ln = ln->l.next;
	}

	return   NULL;
}

