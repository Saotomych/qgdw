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
#include "hmi.h"

#define MENUFILE	1
#define MENUMEM		2

#define MENUSTEP	16

typedef struct _item{
	GR_WINDOW_ID main_window;
	GR_RECT rect;
	GR_RECT bgnrect;
	varrec *vr;			// pointer to variable of item
	char *name_font;
	char *text;			// Start text of item
	char *endtext;		// End text of item
	char *next_menu;  	// pointer to next submenu
	char *action;
	char next_item;		// index of next menu point
	char prev_item;		// index of previous menu point
}item;

typedef struct _menu{
	GR_WINDOW_ID main_window;
	GR_FONT_ID	font;
	item **pitems;
	char *ptxtmenu;     // pointer to menu dimension of texts
	int num_item;   //текущий пункт меню
	int start_item; //пункт начала вывода
	int count_item;        //number of item
	int bgnmenuy;	// first y coord of 1th menu item
	char first_item;	// index of first menu point
//  функции set

}menu;

extern int init_menu(fact *factsetting, int len);
extern int do_openfilemenu(char *buf, int type);
extern void draw_menu();
extern void f1(void *arg);
extern void f2(void *arg);
extern void f3(void *arg);


//void SetNewMenu();
//void SetMainMenu();
//void HintDraw();
//void EnInfoDraw();

#endif

