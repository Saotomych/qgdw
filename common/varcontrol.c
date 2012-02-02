/*
 * varcontrol.c
 *
 *  Created on: 27.01.2012
 *      Author: Alex AVAlon
 */

#include "common.h"
#include "multififo.h"
#include "asdu.h"
#include "varcontrol.h"

static LIST fdefvt   = {NULL, NULL};		// first  varrec
static varrec *defvt;		// actual varrec

static LIST fvarbook = {NULL, NULL};		// first varbook
static varbook *actvb;		// actual varbook

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

	// Create const var table
	defvt = (varrec*) &fdefvt;
	for (i=0; i < len; i++){
		defvt = create_next_struct_in_list((LIST*) &defvt->l, sizeof(varrec));
		defvt->name = malloc(sizeof(fcdarec));
		defvt->val = vt[i];
		defvt->name->fc = defvt->val.name;
		defvt->prop = INTVAR | TRUEVALUE;
		defvt->time = 0;
	}


		// Bring to conformity with all internal variables and config variables
		// It's equal constant booking

	// Multififo init


}

// To book concrete variable by name
// Return pointer to value and her properties
varrec *vc_addvarrec(char *varname){

	// if APP - Find pointer in table

	// if IED, LD - Set var as text

	// if LN without DO/DA.name - Set var as text

	// if LN has DO.name - book this variable

	return NULL;
}

// To delete booking of concrete variable by name
int vc_rmvarrec(char *varname){

	return 0;
}

void vc_checkvars(){

}
