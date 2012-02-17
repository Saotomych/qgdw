/*
 * winparser.c
 *
 *  Created on: 13.02.2012
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "menu.h"

static menu *allmenus[MAXMENU];
static int maxmenus = 0;

extern LNODE *actlnode;

//------------------------------------------------------------------------------------
// Definition for function keys
extern void default_left(GR_EVENT *event);
extern void default_right(GR_EVENT *event);
extern void default_up(GR_EVENT *event);
extern void default_down(GR_EVENT *event);
extern void default_enter(GR_EVENT *event);
extern void date_left(GR_EVENT *event);
extern void date_right(GR_EVENT *event);
extern void date_up(GR_EVENT *event);
extern void date_down(GR_EVENT *event);
extern void date_enter(GR_EVENT *event);

struct _kt {
	char *funcname;
	void (*func)();
} keystable[] = {
		{"null", (void*) -1},
		{"defleft", default_left},
		{"defright", default_right},
		{"defup", default_up},
		{"defdown", default_down},
		{"defenter", default_enter},
		{"dateleft", date_left},
		{"dateright", date_right},
		{"dateup", date_up},
		{"datedown", date_down},
		{"dateenter", date_enter},
		{"", NULL},
};

void* find_keyname(char *p){
int x = 0, kt = 0;
char *bgnp = p;

	while (keystable[kt].funcname[0]){
		x = 0; p = bgnp;
		while ((keystable[kt].funcname[x] == *p) && (*p)){
			p++; x++;
		}
		if ((*p <= ' ') && (!keystable[kt].funcname[x])){
			return keystable[kt].func;
		}
		kt++;
	}

	return NULL;
}

//------------------------------------------------------------------------------------
// Parser of menu files
// It's making:
// Create and initialize structure menu and structure array of items
// Connect items to variables
// Fill items by begin graphic information
// In: file or string array with string as <type> <X> <Y> <Width> <Height> <Fix text with variables> <'>'submenu> <'~'action>
// Out: num_menu pointer and all connected pointers are ready for next work
menu* do_openfilemenu(char *buf){
char *pitemtype;			// pointer to actual <type>
char *ptxt;					// temporary text pointer
int count_item = 0;			// max number of items
int count_menu = 0;			// max number of items as menu type
char last_menuitem = 0;		// number of last item as menu type
char first_menuitem = 0;	// number of first item as menu type
int clen;

int i;
char *p;

char servsyms[] = {" >~"};

menu *num_menu = allmenus[maxmenus];
varrec *nextvarrec;


			if ((maxmenus) >= MAXMENU) return num_menu;

			num_menu = malloc(sizeof(menu));   //возвращает указатель на первый байт блока области памяти структуры меню

			clen = strlen(buf);
			if (!clen){
				free(num_menu);
				return 0;
			}

			num_menu->ptxtmenu = malloc(clen + 2);
			strcpy((num_menu->ptxtmenu+1), buf);

		 	//Make ending 0 for string
		 	num_menu->ptxtmenu[clen+1] = 0;
		 	//Make ending start 0
		 	num_menu->ptxtmenu[0] = 0;

		 	for (i=1; i<clen; i++)
	        {
	        	if (num_menu->ptxtmenu[i] == 0xD || num_menu->ptxtmenu[i] == 0xA) num_menu->ptxtmenu[i] = 0;
	        	if ((num_menu->ptxtmenu[i]) && (!num_menu->ptxtmenu[i-1])) count_item++;
	        }

		 	ptxt = num_menu->ptxtmenu;
	 	 	num_menu->bgnmenuy = 0;

	 	 	// Initialize RECT of main window
            while (!(*ptxt)) ptxt++;
            ptxt[4] = 0;
            if (!strcmp(ptxt, "main")){
            	count_item--;
                ptxt += 5;
	            // Parse RECTangle for item
	            // X
	            if (*ptxt != 'a') num_menu->rect.x = atoi(ptxt);
	            else num_menu->rect.x = 0;
//	            while (*ptxt != ' ') ptxt++; ptxt++;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // Y
	            if (*ptxt != 'a') num_menu->rect.y = atoi(ptxt);
	            else num_menu->rect.y = 0;
//	            while (*ptxt != ' ') ptxt++; ptxt++;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // W
	            if (*ptxt != 'a') num_menu->rect.width = atoi(ptxt);
	            else num_menu->rect.width = MAIN_WIDTH;
//	            while (*ptxt != ' ') ptxt++; ptxt++;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // H
	            if (*ptxt != 'a') num_menu->rect.height = atoi(ptxt);
	            else num_menu->rect.height = MAIN_HEIGHT;
	            ptxt += strcspn(ptxt, servsyms) + 1;
          }else{
            	num_menu->rect.x = 0;
            	num_menu->rect.y = 0;
            	num_menu->rect.width = MAIN_WIDTH;
            	num_menu->rect.height = MAIN_HEIGHT;
            }

            // Initialize keys functions
            num_menu->keyup = default_up;
            num_menu->keydown = default_down;
            num_menu->keyright = default_right;
            num_menu->keyleft = default_left;
            num_menu->keyenter = default_enter;
            ptxt[4] = 0;
            if (!strcmp(ptxt, "keys")){
            	count_item--;
            	ptxt += 5;
            	p = strstr(ptxt, "up:");
                if (p){
                	p += 3;
                	num_menu->keyup = find_keyname(p);
                	if (num_menu->keyup == NULL) num_menu->keyup = default_up;
                	if (num_menu->keyup == (void*) -1) num_menu->keyup = NULL;
                }

                p = strstr(ptxt, "down:");
                if (p){
                	p += 5;
                	num_menu->keydown = find_keyname(p);
                	if (num_menu->keydown == NULL) num_menu->keydown = default_down;
                	if (num_menu->keydown == (void*) -1) num_menu->keydown = NULL;
                }

        		p = strstr(ptxt, "right:");
                if (p){
                	p += 6;
                	num_menu->keyright = find_keyname(p);
                	if (num_menu->keyright == NULL) num_menu->keyright = default_right;
                	if (num_menu->keyright == (void*) -1) num_menu->keyright = NULL;
                }

        		p = strstr(ptxt, "left:");
                if (p){
                	p += 5;
                	num_menu->keyleft = find_keyname(p);
                	if (num_menu->keyleft == NULL) num_menu->keyleft = default_left;
                	if (num_menu->keyleft == (void*) -1) num_menu->keyleft = NULL;
                }

                p = strstr(ptxt, "enter:");
                if (p){
                	p += 6;
                	num_menu->keyenter = find_keyname(p);
                	if (num_menu->keyenter == NULL) num_menu->keyenter = default_enter;
                	if (num_menu->keyenter == (void*) -1) num_menu->keyenter = NULL;
                }

                while (*ptxt >= ' ') ptxt++; ptxt++;

            }

		 	if (count_item > MAXITEM) count_item = MAXITEM;
		 	num_menu->pitems = malloc(count_item * sizeof(item*));
		 	nextvarrec = (varrec*) &(num_menu->fvarrec);

		 	for (i = 0; i < count_item; i++){
	            while ((*ptxt) < ' ') ptxt++;

	            num_menu->pitems[i] = (item*) malloc(sizeof(item));
	            memset(num_menu->pitems[i], sizeof(item), 0);
	            ptxt[4] = 0;
	            pitemtype = ptxt;
	            ptxt += 5;
	            while ((*ptxt) < ' ') ptxt++;

	            // Parse RECTangle for item
	            // X
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.x = atoi(ptxt);
	            else num_menu->pitems[i]->rect.x = 0;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // Y
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.y = atoi(ptxt);
	            else num_menu->pitems[i]->rect.y = num_menu->pitems[i-1]->rect.y + num_menu->pitems[i-1]->rect.height;
  		 		ptxt = strchr(ptxt, ' ') + 1;

	            // W
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.width = atoi(ptxt);
	            else num_menu->pitems[i]->rect.width = num_menu->rect.width - num_menu->pitems[i]->rect.x;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // H
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.height = atoi(ptxt);
	            else num_menu->pitems[i]->rect.height = MENUSTEP;
	            num_menu->pitems[i]->ctrl_height = num_menu->pitems[i]->rect.height;
		 		ptxt = strchr(ptxt, ' ') + 1;

	            // Item type "MENU"
	            if (!strcmp(pitemtype, "menu")){

	            	num_menu->pitems[i]->prev_item = last_menuitem;
	            	num_menu->pitems[i]->next_item = first_menuitem;
	            	num_menu->pitems[(int)last_menuitem]->next_item = i;
		            num_menu->pitems[i]->text = ptxt;
		            num_menu->pitems[i]->next_menu = 0;
	               	num_menu->pitems[i]->action = 0;
	               	p = strpbrk(ptxt, &servsyms[1]); if (p) ptxt = p;
		            do{
		               	p = strpbrk(ptxt, &servsyms[1]);  if (p) ptxt = p;
			            // Spase if border of field in parameters
//			            if (*ptxt == servsyms[0]){
//			            	// End of variable definition
//			            	*ptxt = 0;
//			            	ptxt++;
//			            }
			            if (*ptxt == servsyms[1]){
			            	// Set next submenu
			            	*ptxt = 0;
			            	ptxt++;
			            	num_menu->pitems[i]->next_menu = ptxt;
			            }
			            if (*ptxt == servsyms[2]){
				            // Set next action for left & right keys
			            	*ptxt = 0;
			            	ptxt++;
			               	num_menu->pitems[i]->action = ptxt;
			            }
		            }while (p);
		            last_menuitem = i;
		            count_menu++;
	            }
	            // For fixed first menu point in items
	            if (!count_menu){
	            	last_menuitem++;
	            	first_menuitem++;
	            }

	            // Item type "TEXT"
	            if (!strcmp(pitemtype, "text")){
		            num_menu->pitems[i]->text = ptxt;
	            	num_menu->pitems[i]->prev_item = first_menuitem;
	            	num_menu->pitems[i]->next_item = first_menuitem;
		            num_menu->pitems[i]->next_menu = 0;
	               	num_menu->pitems[i]->action = 0;
		            // menu item included in heigth all lower text field before next menu item
		            if (count_menu) num_menu->pitems[(int)last_menuitem]->ctrl_height += num_menu->pitems[i]->rect.height;
	            }

	            ptxt += strlen(ptxt);

	            // Delete nonsymbolic ending
	            p = ptxt;
	            do{
	            	*p = 0;
	            	p--;
	            }while ((uchar) (*p)  < ' ');

	            // IF Fixed text is variable
	            p = strstr(num_menu->pitems[i]->text, "&var:");
	            if (p){
	            	*p = 0;
	            	p += 5;
	            	num_menu->pitems[i]->vr = vc_addvarrec(actlnode, p, nextvarrec, defvalues);
	            	if (num_menu->pitems[i]->vr) nextvarrec = num_menu->pitems[i]->vr;
	            	while ((*p != ' ') && (*p)) p++;
	            	num_menu->pitems[i]->endtext = p;
	            }else{
	            	num_menu->pitems[i]->vr = NULL;
	            	num_menu->pitems[i]->endtext = NULL;
	            }
	         }

	         num_menu->pitems[(int) first_menuitem]->prev_item = last_menuitem;
	         num_menu->num_item = first_menuitem;
	         num_menu->first_item = first_menuitem;
	         num_menu->start_item = first_menuitem;
	         num_menu->count_item = count_item;
	         num_menu->bgnmenuy = num_menu->pitems[(int) num_menu->first_item]->rect.y;

	         allmenus[maxmenus] = num_menu;
   	 	 	 maxmenus++;

   	 	 	 return num_menu;
}

// -----------------------------------------------------------------------------------
// Full delete menu (num_menu) from memory
menu* destroy_menu(int direct){
int i;
menu *num_menu;

	if (!maxmenus) return 0;
	if (direct == DIR_FORWARD) return 0;	// Never closing lower window
	if ((direct == DIR_BACKWARD) && (maxmenus == 1)) return allmenus[0];

	num_menu = allmenus[maxmenus-1];

	for (i=0; i < num_menu->count_item; i++){
		GrUnmapWindow(num_menu->pitems[i]->main_window);
		GrDestroyWindow(num_menu->pitems[i]->main_window);
		free(num_menu->pitems[i]);
		num_menu->pitems[i] = 0;
	}
	GrUnmapWindow(num_menu->main_window);
	GrDestroyWindow(num_menu->main_window);
	free(num_menu->ptxtmenu);
	free(num_menu);

	maxmenus--;
	allmenus[maxmenus] = 0;

	return (maxmenus ? allmenus[maxmenus-1] : 0);
}
