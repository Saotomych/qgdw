/*
 * action_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "../common/tarif.h"
#include "hmi.h"
#include "menu.h"

extern LNODE *actlnode;
extern tarif *acttarif;

extern time_t maintime;		// Actual Time
extern time_t jourtime;		// Time for journal setting
extern int idlnmenuname;
extern int *pinterval;
extern int intervals;

// Values for change visible lnode types
// For indication
static char lntypes[][50] = {
		{"Телеизмерения"},
		{"Телесигнализация"},
		{"Телеуправление"},
};
// For filter
char lnclasses[][5] = {
		{"MMXU"},
		{"MSQI"},
		{"MMTR"},
};

static void refreshvars(menu *actmenu){
int i, idx, lnclassnum;

// Set ID main menu of type ln
	for (lnclassnum = 0; lnclassnum < 3; lnclassnum++){
		if (!strcmp(lnclasses[lnclassnum], actlnode->ln.lnclass)) break;
	}
	idlnmenuname = lnclassnum;

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
					*((int*)(actmenu->pitems[i]->vr->val->val)) = (int) lntypes[lnclassnum];
				}
				if (idx == 29){
					if (acttarif->id){
						*((int*)(actmenu->pitems[i]->vr->val->val)) = (int) &acttarif->id;
					}else *((int*)(actmenu->pitems[i]->vr->val->val)) = 0;
				}
				if (idx == 30){
					*((int*)(actmenu->pitems[i]->vr->val->val)) = (int) acttarif->name;
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
				return REDRAW;
			}
		}while (pln->l.next);
	}

	return 0;
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
				return REDRAW;
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
	while ((pln) && (strcmp(lntype, pln->ln.lnclass))) pln = pln->l.next;
	if (pln) *pbln = pln;

	return REMAKEMENU;
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
	while ((pln) && (strcmp(lntype, pln->ln.lnclass))) pln = pln->l.next;
	if (pln) *pbln = pln;

//	destroy_menu(DIR_SIDEBKW);
//	num_menu = create_menu(lnmenunames[i]);

	return REMAKEMENU;
}

int next_day(void *arg){
time_t time = (time_t) arg;
struct tm *ttm;

	time += (24 * 60 * 60);
	ttm = localtime(&time);

	return time;
}

int prev_day(void *arg){
time_t time = (time_t) arg;
struct tm *ttm;

	time -= (24 * 60 * 60);
	ttm = localtime(&time);

	return time;
}

int next_jourday(void *arg){

	jourtime = next_day((void*) jourtime);

	return REDRAWTIMEJOUR;
}

int prev_jourday(void *arg){

	jourtime = prev_day((void*) jourtime);

	return REDRAWTIMEJOUR;
}

int next_mainday(void *arg){

	maintime = next_day((void*) maintime);

	return REDRAWTIMEMAIN;
}

int prev_mainday(void *arg){

	maintime = prev_day((void*) maintime);

	return REDRAWTIMEMAIN;
}

int next_min(void *arg){
time_t time = (time_t) arg;
struct tm *ttm;

	time += 60;
	ttm = localtime(&time);

	return time;
}

int prev_min(void *arg){
time_t time = (time_t) arg;
struct tm *ttm;

	time -= 60;
	ttm = localtime(&time);

	return time;
}

int next_jourmin(void *arg){

	jourtime = next_min((void*) jourtime);

	return REDRAWTIMEJOUR;
}

int prev_jourmin(void *arg){

	jourtime = prev_min((void*) jourtime);

	return REDRAWTIMEJOUR;
}

int next_mainmin(void *arg){

	maintime = next_min((void*) maintime);

	return REDRAWTIMEMAIN;
}

int prev_mainmin(void *arg){

	maintime = prev_min((void*) maintime);

	return REDRAWTIMEMAIN;
}

int prev_interval(void *arg){

	if (pinterval > &intervals) pinterval--;
	else pinterval = &intervals + 6;

	return REDRAW;
}

int next_interval(void *arg){

	if (pinterval < (&intervals+6)) pinterval++;
	else pinterval = &intervals;		// Time Interval for view journal records

	return REDRAW;
}

int prev_tarif(void *arg){

	if (acttarif->l.prev) acttarif = acttarif->l.prev;

	return REDRAW;
}

int next_tarif(void *arg){

	if (acttarif->l.next) acttarif = acttarif->l.next;

	return REDRAW;
}

// Array of structures "synonym to function"
fact actfactset[] = {
		{"changeln", (void*) prev_ln, (void*) next_ln},
		{"changetypeln", (void*) prev_type_ln, (void*) next_type_ln},
		{"change1day", (void*) prev_jourday, (void*) next_jourday},
		{"change1min", (void*) prev_jourmin, (void*) next_jourmin},
		{"change1mainday", (void*) prev_mainday, (void*) next_mainday},
		{"change1mainmin", (void*) prev_mainmin, (void*) next_mainmin},
		{"chinterval", (void*) prev_interval, (void*) next_interval},
		{"chtarif", (void*) prev_tarif, (void*) next_tarif},
};

//---*** External IP ***---//
int call_action(int direct, menu *actmenu){
int ret = 0;
int i;
char *act = actmenu->pitems[actmenu->num_item]->action;


	if (actmenu->fvarrec) get_argindex((varrec*) actmenu->fvarrec, 0, 0);	// Set first varrec to var parser

	// Separate keys and call action
	switch(direct){
	case 0xf800:
				 for (i=0; i < sizeof(actfactset) / sizeof(fact); i++){
					if (!strcmp(actfactset[i].action, act)){
						ret = actfactset[i].proloque(actmenu);
					}
				 }
				 break;
	case 0xf801:
				 for (i=0; i < (sizeof(actfactset) / sizeof(fact)); i++){
					 if (!strcmp(actfactset[i].action, act)){
						 ret = actfactset[i].epiloque(actmenu);
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
				return ln;
			}
		}
		ln = ln->l.next;
	}

	return   NULL;
}

