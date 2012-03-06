/*
 * test_window.h
 *
 *  Created on: 27.10.2011
 *      Author: dmitry
 */
#ifndef MENU_H_
#define MENU_H_

#include "../common/common.h"
#include "../common/varcontrol.h"
#include <nano-X.h>
#include <nanowm.h>

#define MENUFILE	1
#define MENUMEM		2
#define MENUSTEP	16

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
#define FONTNAME "7x13.pcf.gz"

#define MAXMENU 5
#define MAXITEM 40

#define REMAKEMENU		100
#define REDRAWTIMEJOUR	101
#define REDRAW			102
#define REDRAWTIMEMAIN	103

#define NODIRECT		0
#define DIR_FORWARD		1
#define DIR_BACKWARD 	2
#define DIR_SIDEFWD		3
#define DIR_SIDEBKW		4

typedef struct _item{
	GR_WINDOW_ID main_window;
	GR_RECT rect;
	GR_RECT bgnrect;
	varrec *vr;			// pointer to variable of item
	int  ctrl_height;	// Height of below control by this menu item
	char *name_font;
	char *text;			// Start text of item
	char *endtext;		// End text of item
	char *next_menu;  	// pointer to next submenu
	char *action;
	char next_item;		// index of next menu point
	char prev_item;		// index of previous menu point
} item;

typedef struct _menu{
	GR_WINDOW_ID main_window;
	GR_FONT_ID	font;
	GR_RECT rect;
	char *fontname;
	item **pitems;
	char *ptxtmenu;     // pointer to menu dimension of texts
	int num_item;   //текущий пункт меню
	int start_item; //пункт начала вывода
	int count_item;        //number of item
	int bgnmenuy;	// first y coord of 1th menu item
	char first_item;	// index of first menu point
//  keys functions
	void (*keyleft)();
	void (*keyright)();
	void (*keyup)();
	void (*keydown)();
	void (*keyenter)();
	varrec *fvarrec;		// Pointer to LIST first varrec in list all varrec for this menu
} menu;

extern int *args;
extern value defvalues[];

extern void menu_parser(pvalue vt, int len);
extern int init_menu(void);
extern void redraw_screen(void *arg);
extern void key_pressed(void *arg);
extern void key_rised(void *arg);

extern int call_action(int direct, menu *actmenu);
LNODE* setdef_lnode(int lnclass, menu *actmenu);

extern menu* create_menu(char *menuname);			// Create texts and structures of menu, call proloque
extern menu* destroy_menu(menu *menu, int direct);	// Call epiloque, Free all structures of menu
extern void call_epiloque(menu *actmenu);

#endif

