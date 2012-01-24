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

typedef struct _item{
	GR_WINDOW_ID main_window;
	char *name_font;
	char *text;
	char *next_menu;  //указатель на следующее меню

}item;

typedef struct _menu{
	GR_WINDOW_ID main_window;
//	int *item_structure;
	item **pitems;
	int num_item;   //текущий пункт меню
	int start_item; //пункт начала вывода
	//int last_item;  //пункт конца вывода
//  функции set

}menu;

extern int init_menu(fact *factsetting, int len);
extern int do_openfilemenu();
extern void draw_menu();
extern void f1(void *arg);
extern void f2(void *arg);
extern void f3(void *arg);


//void SetNewMenu();
//void SetMainMenu();
//void HintDraw();
//void EnInfoDraw();

#endif

