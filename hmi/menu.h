/*
 * test_window.h
 *
 *  Created on: 27.10.2011
 *      Author: dmitry
 */
#ifndef MENU_H_
#define MENU_H_

#ifndef HMI_H_
#include "hmi.h"
#endif

#define MENUFILE	1
#define MENUMEM		2

#define MENUSTEP	32

typedef struct _item{
	GR_WINDOW_ID main_window;
	GR_RECT rect;
	GR_RECT bgnrect;
	char *name_font;
	char *text;
	char *next_menu;  //указатель на имя следующего подменю
	char *action;
	char next_item;		// index of next menu point
	char prev_item;		// index of previous menu point
}item;

typedef struct _menu{
	GR_WINDOW_ID main_window;
	GR_FONT_ID	font;
	item **pitems;
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

