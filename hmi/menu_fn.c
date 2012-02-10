/*
 * menu_fn.c
 *
 *  Created on: 06.02.2012
 *      Author: alex
 */

#include "../common/common.h"
#include "../common/iec61850.h"
#include "hmi.h"
#include "menu.h"

char menutxt[1024];

char* ChangeLN(char *arg){
char *pmenu = menutxt;
LNODE **pbln = ((LNODE**) *((int*)arg));
LNODE *pln = *pbln;
char **lnodefilter = (char**) ((int*)arg)[1];
void **dynmenuvar = (void*)(((int*)arg)[3]);
int i = 1; 	// Real parameters for dynmenu begin from 1
int x, y = 0;

	*dynmenuvar = pbln;

	pln = (LNODE*) fln.next; x = 0;
	sprintf(pmenu, "text %d %d a a Выбор устройства\n", x, y);
	pmenu += strlen(pmenu);
	y += MENUSTEP; x = MENUSTEP;
	while(pln){
		if (!strcmp(pln->ln.lnclass, *lnodefilter)){
			sprintf(pmenu, "menu %d %d a a %s.%s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.ldinst, pln->ln.lnclass, pln->ln.lninst);
			pmenu += strlen(pmenu);
			y += MENUSTEP;
			// Set pointer to LNODE for the item in future
			dynmenuvar[i] = (int*) pln; i++;
		}
		pln = pln->l.next;
	}

	return menutxt;
}

char* ChangeLNType(char *arg){
char *ptxt;

	return ptxt;
}

char* ChangeDate(char *arg){
char *ptxt;

	return ptxt;
}

char* ChangeIntl(char *arg){
char *ptxt;

	return ptxt;
}

char* ChangeTarif(char *arg){
char *ptxt;

	return ptxt;
}

// Array of structs "synonym to function"
fact menufactset[] = {
		{"lnmenu", (void*) ChangeLN},
		{"lntypemenu", (void*) ChangeLNType},
		{"date", (void*) ChangeDate},
		{"interval", (void*) ChangeIntl},
		{"tarif", (void*) ChangeTarif},
};

//---*** External IP ***---//

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
char* create_dynmenu(char *menuname, void *arg){
char *pmenu;
int i;

	pmenu = strstr(menuname,"/");
	if (pmenu) pmenu++;
	else pmenu = menuname;

	for (i=0; i < sizeof(menufactset) / sizeof(fact); i+=2){
		if (!strcmp(menufactset[i].action, pmenu)){
			pmenu = 0;
			pmenu = menufactset[i].func(arg);
		}
	}

	return pmenu;
}

