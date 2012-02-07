/*
 * action_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: alex
 */
#include "../common/common.h"
#include "../common/iec61850.h"
#include "hmi.h"

//---*** Set of action functions ***---//

int next_ln(void *arg){
LNODE **pbln = ((LNODE*) *((int*)arg));
LNODE *pln = *pbln;
char *filter = (char*)((int*)arg)[1];

    if (pln->l.next){
		do{
			pln = pln->l.next;
			if (!strcmp(pln->ln.lnclass, filter)){
				*pbln = pln;
				break;
			}
		}while (pln->l.next);
	}

	return 0;
}

int prev_ln(void *arg){
LNODE **pbln = ((LNODE*) *((int*)arg));
LNODE *pln = *pbln;
char *filter = (char*)((int*)arg)[1];

	if (pln->l.prev){
		do{
			if (pln->l.prev == &fln) break;
			pln = pln->l.prev;
			if (!strcmp(pln->ln.lnclass, filter)){
				*pbln = pln;
				break;
			}
		}while (pln->l.prev);
	}

	return 0;
}

char* prev_type_ln(void *arg){
char *filt;

	return filt;
}

char* next_type_ln(void *arg){
char *filt;

	return filt;
}

// Array of structures "synonym to function"
fact actfactset[] = {
		{"changeln", (void*) prev_ln},      			//0 - left
		{"changeln", (void*) next_ln},					//1 - right
		{"changetypeln", (void*) prev_type_ln},			//2 - left
		{"changetypeln", (void*) next_type_ln},			//3 - right
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
						return actfactset[i].func(arg);
					}
				 }
				 break;
	case 0xf801:
				 for (i=1; i < (sizeof(actfactset) / sizeof(fact) - 1); i+=2){
					 if (!strcmp(actfactset[i].action, act)){
						 return actfactset[i].func(arg);
					 }
				 }
				 break;
	}

	return ret;
}
