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

void SetNewMenu(char *arg){

}

void SetMainMenu(char *arg){

}

void LNMenuDraw(char *arg){

}

void ChNameLN(char *arg){

}

// Array of structs "synonym to function"
fact menufactset[] = {
		{"mainmenu", (void*) SetMainMenu},      //0
		{"newmenu", (void*) SetNewMenu},		//1
		{"lnmenu", (void*) LNMenuDraw},			//2
		{"changeln", (void*) ChNameLN},			//3
};

//---*** External IP ***---//

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
char* create_dynmenu(char *menuname, void *arg){
char menutxt[1024];
char *pmenu = menutxt;
LNODE *pln;
int x = MENUSTEP;
int y = MENUSTEP/2;

	// Find function of dynamic menu
	if (!strcmp("menus/lnmenu", menuname)){

		pln = (LNODE*) fln.next;
		while(pln){
			if (!strcmp(pln->ln.lnclass, "MMXU")){
				sprintf(pmenu, "menu %d %d a a %s.%s%s >item ~changetypeln\n", x, y, pln->ln.prefix, pln->ln.lnclass, pln->ln.lninst);
				pmenu += strlen(pmenu);
				x += 2;
				y += MENUSTEP;
			}
			pln = pln->l.next;
		}
		do_openfilemenu(menutxt, MENUMEM);
		draw_menu();
	}

	return pmenu;
}

