/*
 * hmi.c
 *
 *  Created on: 16.12.2011
 *      Author: dmitry & Alex AVAlon
 */

#include <nano-X.h>
#include <nanowm.h>
#include "../common/common.h"
#include "../common/varcontrol.h"
#include "../common/multififo.h"
#include "../common/iec61850.h"
#include "menu.h"

static LNODE *actlnode;
static time_t time_l;
static struct tm time_tm;

static time_t acttime;

static char prev_item;		// pointer to item in main menu
static menu *num_menu;     //указатель на структуру меню


static char newmenu[40];			// For temporary operations

//static int *dynmenuvar;	// Variable for change by dynmenu; flag of dynmenu state
static int *dynmenuvars[40];	// Pointers to variables according to the menu item

static char *lnodefilter;		// Type of actual lnode
static char *devtypetext;		// Text of device type

struct _parameters{
	LNODE *pln;			// Address of pointer of actlnode
	char  *devtype;		// Pointer to lnodefilter
	char  *devtypetext;	// Pointer to lnodefilter text description
	int	  *dynmenuvars;	// Pointer to variable for changing by actual dynamic menu
	time_t *ptimel;		// Pointer to time as time_t type
	struct tm *ptimetm;	// Pointer to time as struct tm type
} parameters;

#define MAIN_WIDTH 160     //параметры главного окна
#define MAIN_HEIGHT 160

#define WIDTH 160          //параметры окна пункта меню
#define HEIGHT MENUSTEP

#define FONT_WIDTH 7
#define FONT_HEIGHT 13

#define BLACK MWRGB( 0  , 0  , 0   )
#define WHITE MWRGB( 255, 255, 255 )

#define FGCOLOR	BLACK
#define BGCOLOR	WHITE
#define FONTNAME "pcf/7x13.pcf.gz"

value defvalues[] = {
		{"APP:ldtypetext", &devtypetext, "Тип не выбран", PTRSTRING , STRING},
};
//------------------------------------------------------------------------------------
// Parser of menu files
// It's making:
// Create and initialize structure menu and structure array of items
// Connect items to variables
// Fill items by begin graphic information
// In: file or string array with string as <type> <X> <Y> <Width> <Height> <Fix text with variables> <'>'submenu> <'~'action>
// Out: num_menu pointer and all connected pointers are ready for next work
int do_openfilemenu(char *buf, int type){
char *pitemtype;			// pointer to actual <type>
char *ptxt;					// temporary text pointer
FILE *fmcfg;               //file open
struct stat fst;			// statistics of file
int clen;					// lenght of file or array
int count_item = 0;			// max number of items
int count_menu = 0;			// max number of items as menu type
char last_menuitem = 0;		// number of last item as menu type
char first_menuitem = 0;	// number of first item as menu type

int i;
char *p;

	 	 	num_menu = malloc(sizeof(menu));   //возвращает указатель на первый байт блока области памяти структуры меню

    		switch(type){

			case MENUFILE:

							if (stat(buf, &fst) == -1){
								printf("IEC Virt: menufile not found\n");
								free(num_menu);
								return -1;
							}

							num_menu->ptxtmenu =  malloc(fst.st_size + 2);
						 	fmcfg = fopen(buf, "r");
						 	clen = fread((num_menu->ptxtmenu+1), 1, (size_t) (fst.st_size), fmcfg);
						 	if (!clen) return -1;
						 	if (clen != fst.st_size) return -1;
						 	break;

			case MENUMEM:
							clen = strlen(buf);
						 	if (!clen) return -1;

						 	num_menu->ptxtmenu = malloc(clen + 2);
						 	strcpy((num_menu->ptxtmenu+1), buf);
						 	break;
			}


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
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // Y
	            if (*ptxt != 'a') num_menu->rect.y = atoi(ptxt);
	            else num_menu->rect.y = 0;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // W
	            if (*ptxt != 'a') num_menu->rect.width = atoi(ptxt);
	            else num_menu->rect.width = MAIN_WIDTH;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // H
	            if (*ptxt != 'a') num_menu->rect.height = atoi(ptxt);
	            else num_menu->rect.height = MAIN_HEIGHT;
	            while (*ptxt > ' ') ptxt++; ptxt++;
            }else{
            	num_menu->rect.x = 0;
            	num_menu->rect.y = 0;
            	num_menu->rect.width = MAIN_WIDTH;
            	num_menu->rect.height = MAIN_HEIGHT;
            }

		 	num_menu->pitems = malloc(count_item * sizeof(item*)); //возвращает указатель на первый байт области памяти структуры пунктов

		 	for (i = 0; i < count_item; i++){
	            while ((*ptxt) < ' ') ptxt++;

	            num_menu->pitems[i] = (item*) malloc(sizeof(item));
	            memset(num_menu->pitems[i], sizeof(item), 0);
	            ptxt[4] = 0;
	            pitemtype = ptxt;
	            ptxt += 5;
	            while (*ptxt == ' ') ptxt++;

	            // Parse RECTangle for item
	            // X
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.x = atoi(ptxt);
	            else num_menu->pitems[i]->rect.x = 0;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // Y
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.y = atoi(ptxt);
	            else num_menu->pitems[i]->rect.y = num_menu->pitems[i-1]->rect.y + num_menu->pitems[i-1]->rect.height;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // W
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.width = atoi(ptxt);
	            else num_menu->pitems[i]->rect.width = num_menu->rect.width - num_menu->pitems[i]->rect.x;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // H
	            if (*ptxt != 'a') num_menu->pitems[i]->rect.height = atoi(ptxt);
	            else num_menu->pitems[i]->rect.height = MENUSTEP;
	            num_menu->pitems[i]->ctrl_height = num_menu->pitems[i]->rect.height;
	            while (*ptxt != ' ') ptxt++; ptxt++;

	            // Item type "MENU"
	            if (!strcmp(pitemtype, "menu")){

	            	num_menu->pitems[i]->prev_item = last_menuitem;
	            	num_menu->pitems[i]->next_item = first_menuitem;
	            	num_menu->pitems[(int)last_menuitem]->next_item = i;
		            num_menu->pitems[i]->text = ptxt;
		            num_menu->pitems[i]->next_menu = 0;
	               	num_menu->pitems[i]->action = 0;
	               	if (dynmenuvars[0]) num_menu->pitems[i]->dynmenuvar = dynmenuvars[count_menu+1];
	               	else num_menu->pitems[i]->dynmenuvar = 0;
		            while ((*ptxt) && (*ptxt != '>') && (*ptxt != '~')) ptxt++;
		            do{
			            while ((*ptxt != ' ') && (*ptxt) && (*ptxt != '>') && (*ptxt != '~')) ptxt++;
			            // Spase if border of field in parameters
			            if (*ptxt == ' '){
			            	*ptxt = 0;
			            	ptxt++;
			            }
		            	// Set next submenu
			            if (*ptxt == '>'){
			            	*ptxt = 0;
			            	ptxt++;
			            	num_menu->pitems[i]->next_menu = ptxt;
			            }
			            // Set next action for left & right keys
			            if (*ptxt == '~'){
			            	*ptxt = 0;
			            	ptxt++;
			               	num_menu->pitems[i]->action = ptxt;
			            }
		            }while (*ptxt);
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
	               	num_menu->pitems[i]->dynmenuvar = 0;
		            // menu item included in heigth all lower text field before next menu item
		            if (count_menu) num_menu->pitems[(int)last_menuitem]->ctrl_height += num_menu->pitems[i]->rect.height;
	            }

	            ptxt += strlen(ptxt);

	            // IF Fixed text is variable
	            p = strstr(num_menu->pitems[i]->text, "&var:");
	            if (p){
	            	*p = 0;
	            	p += 5;
	            	num_menu->pitems[i]->vr = vc_addvarrec(p, actlnode, NULL);
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
     return 0;
}
//------------------------------------------------------------------------------------
// Function create new text in all windows
static char wintext[40];
void do_paint(item *pitem, int fg, int bg)
{
	        GR_GC_ID gc = GrNewGC();
			GR_WINDOW_ID *main_window = &(pitem->main_window);
			GR_WINDOW_INFO winfo;

			GrGetWindowInfo(*main_window, &winfo);

			gc = GrNewGC();                                   //Allocate a new graphic context
			GrSetGCUseBackground(gc, GR_FALSE);
			GrSetGCBackground(gc, fg);
			GrSetGCForeground(gc, bg);
			GrFillRect(*main_window, gc, 0, 0, winfo.width, winfo.height);
			GrSetGCForeground(gc, fg);

			GrSetGCFont(gc, num_menu->font);

			if (pitem->vr) {
//				if (pitem->vr->prop & ISTRUE)
				if (pitem->vr->prop & INT32){
					sprintf(wintext, "%s%d%s", pitem->text, *((int*) (pitem->vr->val->val)), pitem->endtext);
				}

				if (pitem->vr->prop & INT64){
					sprintf(wintext, "%s%ld%s", pitem->text, *((long*) (pitem->vr->val->val)), pitem->endtext);
				}

				if (pitem->vr->prop & STRING){
					strcpy(wintext, pitem->text);
					if ((pitem->vr) && (pitem->vr->val)){
						if (pitem->vr->val->val) strcat(wintext, (char*) pitem->vr->val->val);
						else if (pitem->vr->val->defval) strcat(wintext, (char*) pitem->vr->val->defval);
					}
					if (pitem->endtext) strcat(wintext, pitem->endtext);
				}

				if (pitem->vr->prop & PTRSTRING){
					strcpy(wintext, pitem->text);
					if ((pitem->vr) && (pitem->vr->val)){
						if (pitem->vr->val->val) strcat(wintext, (char*) *((int*) pitem->vr->val->val));
						else if (pitem->vr->val->defval) strcat(wintext, (char*) pitem->vr->val->defval);
					}
					if (pitem->endtext) strcat(wintext, pitem->endtext);
				}

				GrText(*main_window, gc, 0, 0, wintext, strlen(wintext), GR_TFUTF8|GR_TFTOP);

			}else GrText(*main_window, gc, 0, 0, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

			GrDestroyGC(gc);
}
//------------------------------------------------------------------------------------
// Create subwindows for menu items and map all window
void draw_menu()
{
		if (!num_menu) return;

		 if (GrOpen() < 0) {
			 fprintf(stderr, "Cannot open graphics\n");
			 exit(1);
			}

			wm_init();
			//Paint on the screen window main_window

			GR_WINDOW_ID *main_window;
			GR_WINDOW_INFO winfo;
			GR_WM_PROPERTIES props;


			num_menu->main_window = GrNewWindow(GR_ROOT_WINDOW_ID,
												num_menu->rect.x,
												num_menu->rect.y,
												num_menu->rect.width,
												num_menu->rect.height,
												1, WHITE, BLACK);
   		    GrMapWindow(num_menu->main_window);

			props.flags = GR_WM_FLAGS_PROPS;
			props.props = GR_WM_PROPS_BORDER |
					   GR_WM_PROPS_CAPTION;
			props.title = "";

			GrSetWMProperties(num_menu->main_window, &props);

			GrGetWindowInfo(GR_ROOT_WINDOW_ID, &winfo);
			GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN);
			GrSelectEvents(num_menu->main_window, winfo.eventmask | GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_KEY_DOWN);

			//Paint on the screen windows of item
			int i;

			for (i=0; i < num_menu->count_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				*main_window = GrNewWindow(num_menu->main_window,
										   num_menu->pitems[i]->rect.x,
										   num_menu->pitems[i]->rect.y,
										   num_menu->pitems[i]->rect.width,
										   num_menu->pitems[i]->rect.height,
										   0, WHITE, WHITE);

				props.flags = 0;
				props.props = 0;

				GrSetWMProperties(*main_window, &props);
				GrSelectEvents(*main_window, 0);
				GrMapWindow(*main_window);      //Make a window visible on the screen
	        }

			num_menu->font = GrCreateFontEx(FONTNAME, FONT_HEIGHT, FONT_WIDTH, 0);
			GrSetFontAttr(num_menu->font, GR_TFANTIALIAS | GR_TFKERNING, 0);

}

//-------------------------------------------------------------------------------------------
// Draw only visible items and move items
void redraw_menu(int type){
int i;
int stepy, y, itemy, itemh;

	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->ctrl_height;

	stepy = itemy + itemh - num_menu->rect.height;
	if (!type){
		if (num_menu->num_item == num_menu->first_item) stepy = itemy - num_menu->bgnmenuy;
	}else{
		if (num_menu->num_item < num_menu->pitems[num_menu->num_item]->next_item) stepy = itemy - num_menu->bgnmenuy;
	}

	for (i = num_menu->start_item; i < num_menu->count_item; i++){
		num_menu->pitems[i]->rect.y -= stepy;
		y = num_menu->pitems[i]->rect.y;
		if ((y >= num_menu->bgnmenuy - 10) && (y < num_menu->rect.height)){
			GrMoveWindow(num_menu->pitems[i]->main_window,
					 num_menu->pitems[i]->rect.x,
					 num_menu->pitems[i]->rect.y);
			GrMapWindow(num_menu->pitems[i]->main_window);
		}else GrUnmapWindow(num_menu->pitems[i]->main_window);
	}
}

// -----------------------------------------------------------------------------------
// Full delete menu (num_menu) from memory
void destroy_menu()
{
int i;

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
	num_menu = 0;
}

//-----------------------------------------------------------------------------------------------------------
// Refresh all variables in all items of menu
// Call after change global variables has synonym
void refresh_vars(void){
int i;
	for (i = 0; i < num_menu->count_item; i++){
		if (num_menu->pitems[i]->vr){
			num_menu->pitems[i]->vr = vc_addvarrec(num_menu->pitems[i]->vr->name->fc, actlnode, num_menu->pitems[i]->vr);
		}
	}
}

//-------------------------------------------------------------------------------
// Draw all items and cursor position as last
void redraw_screen(void *arg){
    int i;

	for (i = 0; i < num_menu->count_item; i++){
		do_paint(num_menu->pitems[i], FGCOLOR, BGCOLOR);
	}

	do_paint(num_menu->pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

}

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
// Start dynamic menu
void call_dynmenu(char *menuname){
char *pmenu;

	pmenu = create_dynmenu(menuname, &parameters);
	if (pmenu){
		if (do_openfilemenu(pmenu, MENUMEM)) do_openfilemenu("menus/item", MENUFILE);
	}
	draw_menu();

	// Menu of Date

	// Menu of Time

	// Menu of Interval

	// Menu of Tarif

}

//--------------------------------------------------------------------------------
// Key working function:
// keyleft and keyright working through function set in action_fn.c file
// keyup and key down working as cursor keys forever
// ENTER it's enter to submenu forever or confirmation of changes forever        num_menu->bgnmenuy = num_menu->pitems[(int) num_menu->start_item]->rect.y;

// MENU it's return to previous menu without changes forever
void key_pressed(void *arg){
GR_EVENT *event = (GR_EVENT*) arg;
int itemy, itemh, ret;

	switch(event->keystroke.ch)
	{

	case 0xf800:	// Key left
					if (num_menu->pitems[num_menu->num_item]->action){
						ret = call_action(event->keystroke.ch, num_menu->pitems[num_menu->num_item]->action, &parameters);
						refresh_vars();
						redraw_screen(NULL);
					}
					break;

	case 0xf801:	// Key right
					if (num_menu->pitems[num_menu->num_item]->action){
						ret = call_action(event->keystroke.ch, num_menu->pitems[num_menu->num_item]->action, &parameters);
						refresh_vars();
						redraw_screen(NULL);
					}break;

	case 0xf802:	// Key up

					num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;

//					printf("start %d; num %d; count %d\n", num_menu->start_item, num_menu->num_item, num_menu->count_item);
					//-------------------------------------------------------------------------------

					itemy = num_menu->pitems[num_menu->num_item]->rect.y;
					itemh = num_menu->pitems[num_menu->num_item]->rect.height;

					if ((itemy > (num_menu->rect.height-10)) || (itemy  < (num_menu->bgnmenuy - 10))) redraw_menu(1);

					redraw_screen(NULL);

					break;

	case 0xf803:	// Key down

					num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;

//					printf("start %d; num %d; count %d\n", num_menu->start_item, num_menu->num_item, num_menu->count_item);

					itemy = num_menu->pitems[num_menu->num_item]->rect.y;
					itemh = num_menu->pitems[num_menu->num_item]->rect.height;

					if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

					redraw_screen(NULL);

					break;

	case 0x0D:		// Key ENTER
	case 0x20:		// Dynamic menu change our variable
					if (num_menu->pitems[num_menu->num_item]->dynmenuvar){
						// Set var into pointer dynmenuvar as value in actual item
						*dynmenuvars[0] = (int) num_menu->pitems[num_menu->num_item]->dynmenuvar;
						// and refresh all values (temporary solution)
						call_action(0xf801, "changetypeln", &parameters);
						call_action(0xf800, "changetypeln", &parameters);
						*dynmenuvars[0] = (int) num_menu->pitems[num_menu->num_item]->dynmenuvar;
					}

					// Select new menu
					prev_item = num_menu->num_item;
					if (num_menu->pitems[num_menu->num_item]->next_menu){
						strcpy(newmenu, "menus/");
						strcat(newmenu, num_menu->pitems[num_menu->num_item]->next_menu);
						destroy_menu();
						dynmenuvars[0] = 0;
						if (!do_openfilemenu(newmenu, MENUFILE)){
							draw_menu();
						}else call_dynmenu(newmenu);
					}
					break;

	case 0x1B:		// Key MENU / ESC
					destroy_menu();
					if (!do_openfilemenu("menus/item", MENUFILE)){
						draw_menu();
						num_menu->num_item = prev_item;
					}
					break;

	}

}

//---------------------------------------------------------------------------------
// After release of key
void key_rised(void *arg)
{

}

//---------------------------------------------------------------------------------
//int init_menu(fact *factsetting, int len)
int init_menu(){

	if (!fln.next) return 1;

	time_l = time(NULL);

	// Set of parameters for all variable menu and action functions
	actlnode = (LNODE*) (fln.next);
	parameters.pln = (LNODE*) &actlnode;
	parameters.devtype = (char*) &lnodefilter;
	parameters.devtypetext = (char*) &devtypetext;
	parameters.dynmenuvars = dynmenuvars;
	parameters.ptimel = &time_l;
	parameters.ptimetm = localtime(&time_l);
	// Parameters Ready

	call_action(0xf801, "changetypeln", &parameters);
	call_action(0xf801, "changeln", &parameters);

    return 0;
}

void menu_parser(pvalue vt, int len){
int i, j;

	for (i = 0; i < len; i++){
		for (j = 0; j < sizeof(defvalues)/sizeof(value); j++){
			if (!strcmp(vt[i].name, defvalues[j].name)) memcpy(&vt[i], &defvalues[j], sizeof(value));
		}
	}
}

//---------------------------------------------------------------------------------

