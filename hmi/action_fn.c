/*
 * action_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "hmi.h"

//---*** Set of action functions ***---//
// Function change pointer (arg[0]) to pointer of next LNODE with equal class
int next_ln(void *arg){
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **filter = (char**) ((int*)arg)[1];

    if (pln->l.next){
		do{
			pln = pln->l.next;
			if (!strcmp(pln->ln.lnclass, *filter)){
				*pbln = pln;
				return 0;
			}
		}while (pln->l.next);
	}

	return 1;
}

// Function change pointer (arg[0]) to pointer of previous LNODE with equal class
int prev_ln(void *arg){
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **filter = (char**) ((int*)arg)[1];

	if (pln->l.prev){
		do{
			if (pln->l.prev == &fln) break;
			pln = pln->l.prev;
			if (!strcmp(pln->ln.lnclass, *filter)){
				*pbln = pln;
				return 0;
			}
		}while (pln->l.prev);
	}

	return 1;
}

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
// Function change pointer (arg[0]) to pointer of first LNODE with next class in array of classes
int prev_type_ln(void *arg){
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **lntype = (char**)((int*)arg)[1];
char **lntypetext = (char**) (((int*)arg)[2]);
int i;

	for (i = 0; i < 3; i++){
		if (!strcmp(lnclasses[i], pln->ln.lnclass)) break;
	}
	if (i) i--;
	else i = 2;

	pln = (LNODE*) fln.next;
	while ((strcmp(lnclasses[i], pln->ln.lnclass)) && (pln)) pln = pln->l.next;

	if (pln){
		*pbln = pln;
		*lntype = lnclasses[i];
		*lntypetext = lntypes[i];
		return 0;
	}

	return 1;
}

// Function change pointer (arg[0]) to pointer of first LNODE with previous class in array of classes
int next_type_ln(void *arg){
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **lntype = (char**) (((int*)arg)[1]);
char **lntypetext = (char**) (((int*)arg)[2]);
int i;

	for (i = 0; i < 3; i++){
		if (!strcmp(lnclasses[i], pln->ln.lnclass)) break;
	}
	i++;
	if (i >= 3) i = 0;

	pln = (LNODE*) fln.next;
	while ((strcmp(lnclasses[i], pln->ln.lnclass)) && (pln))
		pln = pln->l.next;

	if (pln){
		*pbln = pln;
		*lntype = lnclasses[i];
		*lntypetext = lntypes[i];
		return 0;
	}

	return 1;
}

// Array of structures "synonym to function"
fact actfactset[] = {
		{"changeln", (void*) prev_ln, NULL},      			// for left
		{"changeln", (void*) next_ln, NULL},					// for right
		{"changetypeln", (void*) prev_type_ln, NULL},			// for left
		{"changetypeln", (void*) next_type_ln, NULL},			// for right
};

//---*** External IP ***---//
int call_action(int direct, char *act, void *arg){
int ret = 0;
int i;
	// Separate keys and call action
	switch(direct){
	case 0xf800:
				 for (i=0; i < sizeof(actfactset) / sizeof(fact); i+=2){
					if (!strcmp(actfactset[i].action, act)){
						ret = actfactset[i].proloque(arg);
					}
				 }
				 break;
	case 0xf801:
				 for (i=1; i < (sizeof(actfactset) / sizeof(fact)); i+=2){
					 if (!strcmp(actfactset[i].action, act)){
						 ret = actfactset[i].proloque(arg);
					 }
				 }
				 break;
	}

	return ret;
}
