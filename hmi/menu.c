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

//static char prev_item;		// pointer to item in main menu
static menu *num_menu;     //указатель на структуру меню


static char newmenu[40];			// For temporary operations

static int *dynmenuvars[MAXITEM];	// Pointers to variables according to the menu item

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


value defvalues[] = {
		{"APP:ldtypetext", &devtypetext, "Тип не выбран", PTRSTRING , STRING},
};


//------------------------------------------------------------------------------------
// Function create new text in all windows
static char wintext[40];
static void do_paint(item *pitem, int fg, int bg){
GR_GC_ID gc;
GR_WINDOW_ID *main_window = &(pitem->main_window);
GR_WINDOW_INFO winfo;
int len = sizeof(wintext) - 1;

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
					strncpy(wintext, pitem->text, sizeof(wintext) - 1);
					len -= strlen(wintext);
					if ((pitem->vr) && (pitem->vr->val)){
						if (pitem->vr->val->val) strncat(wintext, (char*) pitem->vr->val->val, len);
						else if (pitem->vr->val->defval) strncat(wintext, (char*) pitem->vr->val->defval, len);
					}
					len = sizeof(wintext) - 1 - strlen(wintext);
					if ((pitem->endtext) && (len > 0)) strncat(wintext, pitem->endtext, len);
					wintext[sizeof(wintext) - 1] = 0;
				}

				if (pitem->vr->prop & PTRSTRING){
					strncpy(wintext, pitem->text, sizeof(wintext) - 1);
					len -= strlen(wintext);
					if ((pitem->vr) && (pitem->vr->val)){
						if (pitem->vr->val->val) strncat(wintext, (char*) *((int*) pitem->vr->val->val), len);
						else if (pitem->vr->val->defval) strncat(wintext, (char*) pitem->vr->val->defval, len);
					}
					len = sizeof(wintext) - 1 - strlen(wintext);
					if ((pitem->endtext) && (len > 0)) strncat(wintext, pitem->endtext, len);
					wintext[sizeof(wintext) - 1] = 0;
				}

				GrText(*main_window, gc, 2, 0, wintext, strlen(wintext), GR_TFUTF8|GR_TFTOP);

			}else GrText(*main_window, gc, 2, 0, pitem->text, strlen(pitem->text), GR_TFUTF8|GR_TFTOP);

			GrDestroyGC(gc);
}
//------------------------------------------------------------------------------------
// Create subwindows for menu items and map all window
static void draw_menu()
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

			for (i = 0; i < num_menu->start_item; i++)
			{
				main_window = &(num_menu->pitems[i]->main_window);

				*main_window = GrNewWindow(num_menu->main_window,
										   num_menu->pitems[i]->rect.x,
										   num_menu->pitems[i]->rect.y,
										   num_menu->pitems[i]->rect.width,
										   num_menu->pitems[i]->rect.height,
										   0, WHITE, WHITE);

				props.flags = GR_WM_FLAGS_PROPS;
				props.props = GR_WM_PROPS_BORDER;

				GrSetWMProperties(*main_window, &props);
				GrSelectEvents(*main_window, GR_EVENT_MASK_EXPOSURE);
				GrMapWindow(*main_window);      //Make a window visible on the screen
	        }

			for (i = num_menu->start_item; i < num_menu->count_item; i++)
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

			redraw_screen(NULL);
}

//-------------------------------------------------------------------------------------------
// Draw only visible items and move items
static void redraw_menu(int type){
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

	for (i=0; i < num_menu->start_item; i++) GrRaiseWindow(num_menu->pitems[i]->main_window);
}

//-----------------------------------------------------------------------------------------------------------
// Refresh all variables in all items of menu
// Call after change global variables has synonym
static void refresh_vars(void){
int i;
	for (i = 0; i < num_menu->count_item; i++){
		if (num_menu->pitems[i]->vr){
			num_menu->pitems[i]->vr = vc_addvarrec(num_menu->pitems[i]->vr->name->fc, actlnode, num_menu->pitems[i]->vr);
		}
	}
}

//-------------------------------------------------------------------------------
// Form dynamic menu on base cycle string and draw on screen
// Start dynamic menu
static void call_dynmenu(char *menuname){
char *pmenu;

	pmenu = create_dynmenu(menuname, &parameters);
	if (pmenu){
		num_menu = do_openfilemenu(actlnode, pmenu, MENUMEM);
		if (!num_menu) num_menu = do_openfilemenu(actlnode, "menus/item", MENUFILE);
	}
}

//-------------------------------------------------------------------------------
// Keys functions

void default_left(GR_EVENT *event){
	if (num_menu->pitems[num_menu->num_item]->action){
		call_action(event->keystroke.ch, num_menu->pitems[num_menu->num_item]->action, &parameters);
		refresh_vars();
		redraw_screen(NULL);
	}
}

void default_right(GR_EVENT *event){
	if (num_menu->pitems[num_menu->num_item]->action){
		call_action(event->keystroke.ch, num_menu->pitems[num_menu->num_item]->action, &parameters);
		refresh_vars();
		redraw_screen(NULL);
	}
}

void default_up(GR_EVENT *event){
int itemy, itemh;

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->prev_item;
	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->rect.height;

	if ((itemy > (num_menu->rect.height-10)) || (itemy  < (num_menu->bgnmenuy - 10))) redraw_menu(1);

	redraw_screen(NULL);

}

void default_down(GR_EVENT *event){
int itemy, itemh;

	num_menu->num_item = num_menu->pitems[num_menu->num_item]->next_item;
	itemy = num_menu->pitems[num_menu->num_item]->rect.y;
	itemh = num_menu->pitems[num_menu->num_item]->rect.height;

	if ((itemy > (num_menu->rect.height-10)) || (itemy < (num_menu->bgnmenuy - 10))) redraw_menu(0);

	redraw_screen(NULL);

}

void default_enter(GR_EVENT *event){
int dir = DIR_FORWARD;

	if (dynmenuvars[num_menu->num_item + 1]){
		// Set var into pointer dynmenuvar as value in actual item
		*dynmenuvars[0] = (int) dynmenuvars[num_menu->num_item + 1];
		// and refresh all values (temporary solution)
		call_action(0xf801, "changetypeln", &parameters);
		call_action(0xf800, "changetypeln", &parameters);
		*dynmenuvars[0] = (int) dynmenuvars[num_menu->num_item + 1];
		// From dynamic menu - backward
		dir = DIR_BACKWARD;
	}

	// Select new menu
//	prev_item = num_menu->num_item;
	if (num_menu->pitems[num_menu->num_item]->next_menu){
		strcpy(newmenu, "menus/");
		strcat(newmenu, num_menu->pitems[num_menu->num_item]->next_menu);
		if (dir == DIR_BACKWARD){
			num_menu = destroy_menu(dir);
			if (num_menu) refresh_vars();
		}
		memset(dynmenuvars, 0, MAXITEM * sizeof(int));
		if (dir == DIR_FORWARD){
			num_menu = do_openfilemenu(actlnode, newmenu, MENUFILE);
			if (!num_menu) call_dynmenu(newmenu);
			if (num_menu) draw_menu();
		}
	}
}

//--------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
// Draw all items and cursor position as last
void redraw_screen(void *arg){
int i;

	for (i = 0; i < num_menu->count_item; i++){
		do_paint(num_menu->pitems[i], FGCOLOR, BGCOLOR);
	}

	do_paint(num_menu->pitems[num_menu->num_item], BGCOLOR, FGCOLOR);

}

// Key working function:
// keyleft and keyright working through function set in action_fn.c file
// keyup and key down working as cursor keys forever
// ENTER it's enter to submenu forever or confirmation of changes forever
// MENU it's return to previous menu without changes forever
void key_pressed(void *arg){
GR_EVENT *event = (GR_EVENT*) arg;
int i;

	switch(event->keystroke.ch){

	case 0xf800:	// Key left
					if (num_menu->keyleft) num_menu->keyleft((GR_EVENT*) arg);
					break;

	case 0xf801:	// Key right
					if (num_menu->keyright) num_menu->keyright((GR_EVENT*) arg);
					break;

	case 0xf802:	// Key up
					if (num_menu->keyup) num_menu->keyup((GR_EVENT*) arg);
					break;

	case 0xf803:	// Key down
					if (num_menu->keydown) num_menu->keydown((GR_EVENT*) arg);
					break;

	case 0x0D:		// Key ENTER
	case 0x20:		// Dynamic menu change our variable
					if (num_menu->keyenter) num_menu->keyenter((GR_EVENT*) arg);
					break;

	case 0x1B:		// Key MENU / ESC
					memset(dynmenuvars, 0, MAXITEM * sizeof(int));
					num_menu = destroy_menu(DIR_BACKWARD);
					if (!num_menu){
						num_menu = do_openfilemenu(actlnode, "menus/item", MENUFILE);
						if (num_menu){
//							draw_menu();
//							num_menu->num_item = prev_item;
							draw_menu();
						}
					}else{
						redraw_screen(NULL);
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
struct tm *ttm;

	if (!fln.next) return 1;

	// Set actual time in vars
	time_l = time(NULL);
	ttm	= localtime(&time_l);
	memcpy(&time_tm, ttm, sizeof(struct tm));

	// Set of parameters for all variable menu and action functions
	actlnode = (LNODE*) (fln.next);
	parameters.pln = (LNODE*) &actlnode;
	parameters.devtype = (char*) &lnodefilter;
	parameters.devtypetext = (char*) &devtypetext;
	parameters.dynmenuvars = (int*) dynmenuvars;
	parameters.ptimel = &time_l;
	parameters.ptimetm = &time_tm;
	// Parameters Ready

	// Set start LN
	call_action(0xf801, "changetypeln", &parameters);
	// Start LN ready

	// Open first menu
	num_menu = do_openfilemenu(actlnode, "menus/item", MENUFILE);
	draw_menu();

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

